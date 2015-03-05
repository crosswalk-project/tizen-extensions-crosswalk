// Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "application/application_control.h"

#include <gio/gio.h>

#include <algorithm>
#include <cstdlib>

// FIXME: this is temporary hack for API that is private
// this function signature matches the one in capi
extern "C" int service_create_event(bundle* data, struct service_s** service);

bool AddAppControlDataToService(const ApplicationControlData& data,
    service_h service) {
  const std::string& key = data.key();
  const std::vector<std::string>& values = data.values();
  std::unique_ptr<const char*[]> array_of_values(
      new const char*[values.size()]);

  for (unsigned i = 0; i < values.size(); ++i)
    array_of_values[i] = values[i].c_str();

  return service_add_extra_data_array(service, key.c_str(),
      array_of_values.get(), values.size()) == SERVICE_ERROR_NONE;
}

bool AddAppControlDataArrayToService(
    const std::vector<std::unique_ptr<ApplicationControlData>>& array,
    service_h service) {
  for (const auto& item : array)
    if (!AddAppControlDataToService(*item, service))
      return false;
  return true;
}

bundle* DecodeApplicationBundle(const std::string& encoded_bundle) {
  return bundle_decode(reinterpret_cast<const bundle_raw*>(
      encoded_bundle.data()),
      static_cast<int>(encoded_bundle.size()));
}

ApplicationControl::ApplicationControl(const std::string& operation,
    const std::string& uri,
    const std::string& mime,
    const std::string& category)
    : operation_(operation),
      uri_(uri),
      mime_(mime),
      category_(category) {
}

const std::string& ApplicationControl::operation() const {
  return operation_;
}

const std::string& ApplicationControl::mime() const {
  return mime_;
}

const std::string& ApplicationControl::uri() const {
  return uri_;
}

const std::string& ApplicationControl::category() const {
  return category_;
}

const std::vector<std::unique_ptr<ApplicationControlData>>&
    ApplicationControl::app_control_data_array() const {
  return app_control_data_array_;
}

bool ApplicationControl::ReplyResult(
    const std::vector<std::unique_ptr<ApplicationControlData>>& data,
    const std::string& encoded_bundle) const {
  bundle* b = DecodeApplicationBundle(encoded_bundle);
  if (!b)
    return false;
  service_h service;
  service_create_event(b, &service);
  if (!service) {
    bundle_free(b);
    return false;
  }

  service_h reply;
  service_create(&reply);
  if (!AddAppControlDataArrayToService(data, reply)) {
    service_destroy(reply);
    service_destroy(service);
    bundle_free(b);
    return false;
  }

  bool success = service_reply_to_launch_request(reply, service,
      SERVICE_RESULT_SUCCEEDED) == SERVICE_ERROR_NONE;
  service_destroy(reply);
  service_destroy(service);
  bundle_free(b);

  return success;
}

bool ApplicationControl::ReplyFailure(const std::string& encoded_bundle) const {
  bundle* b = DecodeApplicationBundle(encoded_bundle);
  if (!b)
    return false;
  service_h service;
  service_create_event(b, &service);
  if (!service) {
    bundle_free(b);
    return false;
  }

  service_h reply;
  service_create(&reply);
  bool success = service_reply_to_launch_request(reply, service,
      SERVICE_RESULT_FAILED) == SERVICE_ERROR_NONE;
  service_destroy(service);
  bundle_free(b);

  return success;
}

void ApplicationControl::AddAppControlData(
    std::unique_ptr<ApplicationControlData> data) {
  app_control_data_array_.push_back(std::move(data));
}

std::unique_ptr<picojson::value> ApplicationControl::ToJson() const {
  picojson::object object;
  object["operation"] = picojson::value(operation_);
  object["uri"] = picojson::value(uri_);
  object["mime"] = picojson::value(mime_);
  object["category"] = picojson::value(category_);
  picojson::array array;
  for (const auto& i : app_control_data_array_) {
    array.push_back(*i->ToJson());
  }
  object["data"] = picojson::value(array);
  return std::unique_ptr<picojson::value>(new picojson::value(object));
}

std::unique_ptr<ApplicationControl>
ApplicationControl::ApplicationControlFromService(service_h s) {
  char* c_operation = nullptr;
  if (service_get_operation(s, &c_operation) != SERVICE_ERROR_NONE)
    return nullptr;

  std::string operation;
  if (c_operation) {
    operation = c_operation;
    free(c_operation);
  }

  char* c_uri = nullptr;
  if (service_get_uri(s, &c_uri) != SERVICE_ERROR_NONE)
    return nullptr;

  std::string uri;
  if (c_uri) {
    uri = c_uri;
    free(c_uri);
  }

  char* c_mime = nullptr;
  if (service_get_mime(s, &c_mime) != SERVICE_ERROR_NONE)
    return nullptr;

  std::string mime;
  if (c_mime) {
    mime = c_mime;
    free(c_mime);
  }

  char* c_category = nullptr;
  if (service_get_category(s, &c_category) != SERVICE_ERROR_NONE)
    return nullptr;

  std::string category;
  if (c_category) {
    category = c_category;
    free(c_category);
  }

  std::unique_ptr<ApplicationControl> app_control(
      new ApplicationControl(operation, uri, mime, category));

  if (service_foreach_extra_data(s,
      [](service_h service, const char* key, void* user_data) {
        bool is_array = false;

        if (service_is_extra_data_array(service, key, &is_array)
            != SERVICE_ERROR_NONE)
          return false;

        std::unique_ptr<std::vector<std::string>> values(
            new std::vector<std::string>());

        if (is_array) {
          char** value = nullptr;
          int len = 0;
          if (service_get_extra_data_array(service, key, &value, &len)
              != SERVICE_ERROR_NONE)
            return false;

          for (int i = 0; i < len; ++i) {
            values->push_back(value[i]);
            free(value[i]);
          }
          free(value);
        } else {
          char* value = nullptr;
          service_get_extra_data(service, key, &value);
          values->push_back(value);
          free(value);
        }

        ApplicationControl* ac = static_cast<ApplicationControl*>(user_data);
        std::unique_ptr<ApplicationControlData> ptr(
            new ApplicationControlData(key, std::move(values)));
        ac->AddAppControlData(std::move(ptr));
        return true;
      }, app_control.get()) != SERVICE_ERROR_NONE)
    return nullptr;

  return app_control;
}

std::unique_ptr<ApplicationControl>
ApplicationControl::ApplicationControlFromJSON(const picojson::value& value) {
  if (!value.contains("operation"))
    return nullptr;
  const picojson::value& operation_value = value.get("operation");
  if (!operation_value.is<std::string>())
    return nullptr;
  const std::string& operation = operation_value.get<std::string>();

  std::string uri;
  if (value.contains("uri")) {
    const picojson::value& uri_value = value.get("uri");
    if (uri_value.is<std::string>())
      uri = uri_value.get<std::string>();
  }

  std::string mime;
  if (value.contains("mime")) {
    const picojson::value& mime_value = value.get("mime");
    if (mime_value.is<std::string>())
      mime = mime_value.get<std::string>();
  }

  std::string category;
  if (value.contains("category")) {
    const picojson::value& category_value = value.get("category");
    if (category_value.is<std::string>())
      category = category_value.get<std::string>();
  }

  if (!value.contains("data"))
    return nullptr;

  const picojson::value data_value = value.get("data");
  if (!data_value.is<picojson::array>())
    return nullptr;

  const picojson::array& array = data_value.get<picojson::array>();

  std::unique_ptr<ApplicationControl> app_control(
      new ApplicationControl(operation, uri, mime, category));

  for (const auto& d : array) {
    std::unique_ptr<ApplicationControlData> d_ptr =
        ApplicationControlData::ApplicationControlDataFromJSON(d);

    // if parsing ApplicationControlData failed then do not return object
    if (!d_ptr)
      return nullptr;

    app_control->AddAppControlData(std::move(d_ptr));
  }

  return app_control;
}

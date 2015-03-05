// Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "application/application_requested_control.h"

#include <app_service.h>
#include <bundle.h>

// FIXME: this is temporary hack for API that is private
// this function signature matches the one in capi
extern "C" int service_create_event(bundle* data, struct service_s** service);

ApplicationRequestedControl::ApplicationRequestedControl(
    const std::string& caller_id,
    std::unique_ptr<ApplicationControl> app_control)
    : caller_id_(caller_id),
      app_control_(std::move(app_control)) {
}

const ApplicationControl* ApplicationRequestedControl::app_control() const {
  return app_control_.get();
}

std::unique_ptr<picojson::value> ApplicationRequestedControl::ToJson() const {
  picojson::object object;
  object["callerId"] = picojson::value(caller_id_);
  if (app_control_)
    object["appControl"] = *app_control_->ToJson();
  return std::unique_ptr<picojson::value>(new picojson::value(object));
}

std::unique_ptr<ApplicationRequestedControl>
ApplicationRequestedControl::GetRequestedApplicationControl(
    const std::string& encoded_bundle) {
  bundle* b = DecodeApplicationBundle(encoded_bundle);
  if (!b)
    return nullptr;

  service_h service = nullptr;
  service_create_event(b, &service);
  if (!service) {
    bundle_free(b);
    return nullptr;
  }
  char* c_caller_id = nullptr;
  service_get_caller(service, &c_caller_id);
  std::string caller_id = c_caller_id ? c_caller_id : "";
  free(c_caller_id);

  std::unique_ptr<ApplicationRequestedControl> result(
      new ApplicationRequestedControl(caller_id,
          ApplicationControl::ApplicationControlFromService(service)));

  service_destroy(service);
  bundle_free(b);

  return result;
}

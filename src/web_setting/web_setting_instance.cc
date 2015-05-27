// Copyright (c) 2014 Samsung Electronics Co., Ltd. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "web_setting/web_setting_instance.h"

#include <string>
#include <memory>
#include "common/picojson.h"

namespace {

const char kJSCallbackKey[] = "_callback";

double GetJSCallbackId(const picojson::value& msg) {
  assert(msg.contains(kJSCallbackKey));
  const picojson::value& id_value = msg.get(kJSCallbackKey);
  return id_value.get<double>();
}

void SetJSCallbackId(picojson::value& msg, double id) {
  assert(msg.is<picojson::object>());
  msg.get<picojson::object>()[kJSCallbackKey] = picojson::value(id);
}

}  // namespace

WebSettingInstance::WebSettingInstance(WebSettingExtension* extension)
    : extension_(extension) {
}

WebSettingInstance::~WebSettingInstance() {}

void WebSettingInstance::HandleMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cerr << "Error during parsing message: " << err.c_str();
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "SetUserAgentString")
    HandleSetUserAgentString(v);
  else if (cmd == "RemoveAllCookies")
    HandleRemoveAllCookies(v);
  else
    std::cerr << "ASSERT NOT REACHED. \n";
}

void WebSettingInstance::HandleSetUserAgentString(const picojson::value &msg) {
  std::string userAgent = msg.get("userAgentStr").to_str();
  picojson::value *result = extension_->current_app()->
                            SetUserAgentString(userAgent).release();
  ReturnMessageAsync(GetJSCallbackId(msg), *result);
  delete result;
}

void WebSettingInstance::HandleRemoveAllCookies(const picojson::value& msg) {
  picojson::value *result = extension_->current_app()->
                            RemoveAllCookies().release();
  ReturnMessageAsync(GetJSCallbackId(msg), *result);
  delete result;
}

void WebSettingInstance::ReturnMessageAsync(double callback_id,
    picojson::value& value) {
  SetJSCallbackId(value, callback_id);
  PostMessage(value.serialize().c_str());
}

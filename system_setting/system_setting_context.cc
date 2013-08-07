// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_setting/system_setting_context.h"

#include <string>
#include "common/picojson.h"

CXWalkExtension* xwalk_extension_init(int32_t api_version) {
  return ExtensionAdapter<SystemSettingContext>::Initialize();
}

SystemSettingContext::SystemSettingContext(ContextAPI* api)
    : api_(api) {}

SystemSettingContext::~SystemSettingContext() {
  delete api_;
}

const char SystemSettingContext::name[] = "tizen.systemsetting";

// This will be generated from system_setting_api.js.
extern const char kSource_system_setting_api[];

const char* SystemSettingContext::GetJavaScript() {
  return kSource_system_setting_api;
}

void SystemSettingContext::HandleMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "SetProperty")
    HandleSetProperty(v);
  else if (cmd == "GetProperty")
    HandleGetProperty(v);
}

void SystemSettingContext::OnPropertyHandled(const char* reply_id,
                                             const char* value, int ret) {
  picojson::value::object o;
  o["_reply_id"] = picojson::value(reply_id);
  if (value)
    o["_file"] = picojson::value(value);
  o["_error"] = picojson::value(static_cast<double>(ret));

  picojson::value v(o);
  api_->PostMessage(v.serialize().c_str());
}

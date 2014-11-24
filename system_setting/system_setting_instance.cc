// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_setting/system_setting_instance.h"

#include <string>
#include "common/picojson.h"

SystemSettingInstance::SystemSettingInstance() {}

SystemSettingInstance::~SystemSettingInstance() {}

void SystemSettingInstance::HandleMessage(const char* message) {
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
  else
    std::cout << "ASSERT NOT REACHED. \n";
}

void SystemSettingInstance::OnPropertyHandled(const char* reply_id,
                                              const char* value,
                                              int ret) {
  picojson::value::object o;
  o["_reply_id"] = picojson::value(reply_id);
  if (value)
    o["_file"] = picojson::value(value);
  o["_error"] = picojson::value(ret != 0);

  picojson::value v(o);
  PostMessage(v.serialize().c_str());
}

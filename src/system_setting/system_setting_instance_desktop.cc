// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_setting/system_setting_instance.h"
#include "common/picojson.h"

void SystemSettingInstance::HandleSetProperty(const picojson::value& msg) {
  SystemSettingType type = static_cast<SystemSettingType>
    (msg.get("_type").get<double>());
  const char* value = msg.get("_file").to_str().c_str();
  const char* reply_id = msg.get("_reply_id").to_str().c_str();

  OnPropertyHandled(reply_id, value, 0);
}

void SystemSettingInstance::HandleGetProperty(const picojson::value& msg) {
  SystemSettingType type = static_cast<SystemSettingType>
    (msg.get("_type").get<double>());
  const char* reply_id = msg.get("_reply_id").to_str().c_str();

  // FIXME(riju) : Use correct value when desktop version is implemented
  OnPropertyHandled(reply_id, "test.png", 0);
}

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_build.h"

#include <string>
#if defined(TIZEN_MOBILE)
#include <system_info.h>
#endif

#include "common/picojson.h"
#include "system_info/system_info_utils.h"

SysInfoBuild::SysInfoBuild(ContextAPI* api)
    : stopping_(false) {
  api_ = api;
}

SysInfoBuild::~SysInfoBuild() {
}

void SysInfoBuild::Get(picojson::value& error,
                       picojson::value& data) {
  // model and manufacturer
  if (!UpdateHardware()) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get Hardware info failed."));
    return;
  }
  system_info::SetPicoJsonObjectValue(data, "model",
      picojson::value(model_));
  system_info::SetPicoJsonObjectValue(data, "manufacturer",
      picojson::value(manufacturer_));

  // build version
  if (!UpdateOSBuild()) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get Build version failed."));
    return;
  }
  system_info::SetPicoJsonObjectValue(data, "buildVersion",
      picojson::value(buildversion_));

  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

bool SysInfoBuild::UpdateHardware() {
  char* cptr = NULL;
  std::string hardware_info;

  system_info_key_e key = SYSTEM_INFO_KEY_MODEL;
  system_info_get_value_string(key, &cptr);

  hardware_info.assign(cptr);
  if (hardware_info.empty()) {
    return false;
  } else {
    model_.assign(hardware_info);
  }

  cptr = NULL;
  key = SYSTEM_INFO_KEY_MANUFACTURER;
  system_info_get_value_string(key, &cptr);

  hardware_info.assign(cptr);
  if (hardware_info.empty()) {
    return false;
  } else {
    manufacturer_.assign(hardware_info);
  }
  return true;
}

bool SysInfoBuild::UpdateOSBuild() {
  char* cptr = NULL;
  std::string build_info;

  system_info_key_e key = SYSTEM_INFO_KEY_BUILD_STRING;
  system_info_get_value_string(key, &cptr);

  build_info.assign(cptr);
  if (build_info.empty()) {
    return false;
  } else {
    buildversion_.assign(build_info);
    return true;
  }
}

gboolean SysInfoBuild::OnUpdateTimeout(gpointer user_data) {
  SysInfoBuild* instance = static_cast<SysInfoBuild*>(user_data);

  if (instance->stopping_) {
    instance->stopping_ = false;
    return FALSE;
  }

  std::string oldmodel_ = instance->model_;
  std::string oldmanufacturer_ = instance->manufacturer_;
  std::string oldbuildversion_ = instance->buildversion_;
  instance->UpdateHardware();
  instance->UpdateOSBuild();

  if (oldmodel_ != instance->model_ ||
      oldmanufacturer_ != instance->manufacturer_ ||
      oldbuildversion_ != instance->buildversion_) {
    picojson::value output = picojson::value(picojson::object());
    picojson::value data = picojson::value(picojson::object());

    system_info::SetPicoJsonObjectValue(data, "manufacturer",
        picojson::value(instance->manufacturer_));
    system_info::SetPicoJsonObjectValue(data, "model",
        picojson::value(instance->model_));
    system_info::SetPicoJsonObjectValue(data, "buildVersion",
        picojson::value(instance->buildversion_));
    system_info::SetPicoJsonObjectValue(output, "cmd",
        picojson::value("SystemInfoPropertyValueChanged"));
    system_info::SetPicoJsonObjectValue(output, "prop",
        picojson::value("BUILD"));
    system_info::SetPicoJsonObjectValue(output, "data", data);

    std::string result = output.serialize();
    instance->api_->PostMessage(result.c_str());
  }

  return TRUE;
}

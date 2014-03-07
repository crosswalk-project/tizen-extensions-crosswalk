// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_build.h"

#include <stdlib.h>
#if defined(TIZEN)
#include <system_info.h>
#endif

#include "common/picojson.h"

const std::string SysInfoBuild::name_ = "BUILD";

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
  char* hardware_info = NULL;

  if (system_info_get_value_string(SYSTEM_INFO_KEY_MODEL, &hardware_info)
      != SYSTEM_INFO_ERROR_NONE)
    return false;

  if (hardware_info) {
    model_ = hardware_info;
    free(hardware_info);
    hardware_info = NULL;
  } else {
    model_ = "";
  }

  if (system_info_get_value_string(SYSTEM_INFO_KEY_MANUFACTURER, &hardware_info)
      != SYSTEM_INFO_ERROR_NONE)
    return false;

  if (hardware_info) {
    manufacturer_ = hardware_info;
    free(hardware_info);
    hardware_info = NULL;
  } else {
    manufacturer_ = "";
  }

  return true;
}

bool SysInfoBuild::UpdateOSBuild() {
  char* build_info = NULL;

  if (system_info_get_value_string(SYSTEM_INFO_KEY_BUILD_STRING, &build_info)
      != SYSTEM_INFO_ERROR_NONE)
    return false;

  if (build_info) {
    buildversion_ = build_info;
    free(build_info);
    build_info = NULL;
  } else {
    buildversion_ = "";
  }

  return true;
}

gboolean SysInfoBuild::OnUpdateTimeout(gpointer user_data) {
  SysInfoBuild* instance = static_cast<SysInfoBuild*>(user_data);

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

    instance->PostMessageToListeners(output);;
  }

  return TRUE;
}

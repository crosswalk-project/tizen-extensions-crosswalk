// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_battery.h"

#include <vconf.h>
#include <string>

#include "common/picojson.h"
#include "system_info/system_info_utils.h"

SysInfoBattery::SysInfoBattery(ContextAPI* api)
    : level_(0.0),
      charging_(false),
      stopping_(false) {
  api_ = api;
}

SysInfoBattery::~SysInfoBattery() {
}

void SysInfoBattery::Get(picojson::value& error,
                         picojson::value& data) {
  if (!Update(error)) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Battery not found."));
    return;
  }

  SetData(data);
  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

bool SysInfoBattery::Update(picojson::value& error) {
  int level_info = 0;
  int charging_info = 0;

  if (vconf_get_int(VCONFKEY_SYSMAN_BATTERY_CAPACITY, &level_info) != 0)
    return false;

  level_ = static_cast<double>(level_info)/100;

  if (vconf_get_int(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW, &charging_info) != 0)
    return false;

  charging_ = (charging_info == 0) ? false : true;

  return true;
}

gboolean SysInfoBattery::OnUpdateTimeout(gpointer user_data) {
  SysInfoBattery* instance = static_cast<SysInfoBattery*>(user_data);

  if (instance->stopping_) {
    instance->stopping_ = false;
    return FALSE;
  }

  double old_level = instance->level_;
  double old_charging = instance->charging_;
  picojson::value error = picojson::value(picojson::object());
  if (!instance->Update(error)) {
    // Fail to update, wait for next round
    return TRUE;
  }

  if ((old_level != instance->level_) ||
      (old_charging != instance->charging_)) {
    picojson::value output = picojson::value(picojson::object());
    picojson::value data = picojson::value(picojson::object());

    instance->SetData(data);
    system_info::SetPicoJsonObjectValue(output, "cmd",
        picojson::value("SystemInfoPropertyValueChanged"));
    system_info::SetPicoJsonObjectValue(output, "prop",
        picojson::value("BATTERY"));
    system_info::SetPicoJsonObjectValue(output, "data", data);

    std::string result = output.serialize();
    instance->api_->PostMessage(result.c_str());
  }

  return TRUE;
}

void SysInfoBattery::SetData(picojson::value& data) {
  system_info::SetPicoJsonObjectValue(data, "level",
      picojson::value(level_));
  system_info::SetPicoJsonObjectValue(data, "isCharging",
      picojson::value(charging_));
}

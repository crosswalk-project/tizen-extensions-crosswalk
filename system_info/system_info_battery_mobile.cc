// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_battery.h"

#include <string>

#include "common/picojson.h"

SysInfoBattery::SysInfoBattery()
    : level_(0.0),
      charging_(false) {
  pthread_mutex_init(&events_list_mutex_, NULL);
}

SysInfoBattery::~SysInfoBattery() {
  for (SystemInfoEventsList::iterator it = battery_events_.begin();
       it != battery_events_.end(); it++) {
    StopListening(*it);
  }
  pthread_mutex_destroy(&events_list_mutex_);
}

void SysInfoBattery::Get(picojson::value& error,
                         picojson::value& data) {
  int level = 0;
  int charging = 0;

  if (vconf_get_int(VCONFKEY_SYSMAN_BATTERY_CAPACITY, &level) == 0 &&
      vconf_get_int(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW, &charging) == 0) {
    charging_ = (charging == 0) ? false : true;
    level_ = static_cast<double>(level)/100;
  } else {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Battery not found."));
    return;
  }

  SetData(data);
  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

bool SysInfoBattery::Update(picojson::value& error) {
  picojson::value output = picojson::value(picojson::object());
  picojson::value data = picojson::value(picojson::object());

  SetData(data);
  system_info::SetPicoJsonObjectValue(output, "cmd",
      picojson::value("SystemInfoPropertyValueChanged"));
  system_info::SetPicoJsonObjectValue(output, "prop",
      picojson::value("BATTERY"));
  system_info::SetPicoJsonObjectValue(output, "data", data);

  std::string result = output.serialize();
  const char* result_as_cstr = result.c_str();
  AutoLock lock(&events_list_mutex_);
  for (SystemInfoEventsList::iterator it = battery_events_.begin();
       it != battery_events_.end(); it++) {
    (*it)->PostMessage(result_as_cstr);
  }
  return true;
}

void SysInfoBattery::SetData(picojson::value& data) {
  system_info::SetPicoJsonObjectValue(data, "level",
      picojson::value(level_));
  system_info::SetPicoJsonObjectValue(data, "isCharging",
      picojson::value(charging_));
}

void SysInfoBattery::UpdateLevel(double level) {
  if (level_ == level)
    return;

  level_ = level;
  picojson::value error = picojson::value(picojson::object());
  Update(error);
}

void SysInfoBattery::UpdateCharging(bool charging) {
  if (charging_ == charging)
    return;

  charging_ = charging;
  picojson::value error = picojson::value(picojson::object());
  Update(error);
}

void SysInfoBattery::OnLevelChanged(keynode_t* node, void* user_data) {
  double level = static_cast<double>(vconf_keynode_get_int(node))/100;
  SysInfoBattery* battery = static_cast<SysInfoBattery*>(user_data);

  battery->UpdateLevel(level);
}

void SysInfoBattery::OnIsChargingChanged(keynode_t* node, void* user_data) {
  bool charging = vconf_keynode_get_bool(node);
  SysInfoBattery* battery = static_cast<SysInfoBattery*>(user_data);

  battery->UpdateCharging(charging);
}

void SysInfoBattery::StartListening(ContextAPI* api) {
  AutoLock lock(&events_list_mutex_);
  battery_events_.push_back(api);

  if (battery_events_.size() > 1)
    return;

  vconf_notify_key_changed(VCONFKEY_SYSMAN_BATTERY_CAPACITY,
      (vconf_callback_fn)OnLevelChanged, this);
  vconf_notify_key_changed(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW,
      (vconf_callback_fn)OnIsChargingChanged, this);
}

void SysInfoBattery::StopListening(ContextAPI* api) {
  AutoLock lock(&events_list_mutex_);
  battery_events_.remove(api);

  if (!battery_events_.empty())
    return;

  vconf_ignore_key_changed(VCONFKEY_SYSMAN_BATTERY_CAPACITY,
      (vconf_callback_fn)OnLevelChanged);
  vconf_ignore_key_changed(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW,
      (vconf_callback_fn)OnIsChargingChanged);
}

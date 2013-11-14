// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_battery.h"

#include <libudev.h>
#include <algorithm>
#include <string>

#include "common/picojson.h"

const std::string SysInfoBattery::name_ = "BATTERY";

SysInfoBattery::SysInfoBattery()
    : level_(0.0),
      charging_(false),
      timeout_cb_id_(0) {
  udev_ = udev_new();
}

SysInfoBattery::~SysInfoBattery() {
  if (udev_)
    udev_unref(udev_);
  if (timeout_cb_id_ > 0)
    g_source_remove(timeout_cb_id_);
}

void SysInfoBattery::StartListening() {
  // FIXME(halton): Use udev D-Bus interface to monitor.
  if (timeout_cb_id_ == 0) {
    timeout_cb_id_ = g_timeout_add(system_info::default_timeout_interval,
                                   SysInfoBattery::OnUpdateTimeout,
                                   static_cast<gpointer>(this));
  }
}

void SysInfoBattery::StopListening() {
  if (timeout_cb_id_ > 0) {
    g_source_remove(timeout_cb_id_);
    timeout_cb_id_ = 0;
  }
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
  bool found;
  struct udev_enumerate *enumerate;
  struct udev_list_entry *devices, *dev_list_entry;

  enumerate = udev_enumerate_new(udev_);
  udev_enumerate_add_match_subsystem(enumerate, "power_supply");
  udev_enumerate_scan_devices(enumerate);
  devices = udev_enumerate_get_list_entry(enumerate);

  found = false;
  udev_list_entry_foreach(dev_list_entry, devices) {
    const char *path  = udev_list_entry_get_name(dev_list_entry);
    struct udev_device* dev = udev_device_new_from_syspath(udev_, path);

    std::string str_capacity =
        system_info::GetUdevProperty(dev, "POWER_SUPPLY_CAPACITY");
    std::string str_charging =
        system_info::GetUdevProperty(dev, "POWER_SUPPLY_STATUS");
    if (str_capacity.empty() && str_charging.empty()) {
      udev_device_unref(dev);
      continue;
    }

    // Found the battery
    int capacity = std::min(100, atoi(str_capacity.c_str()));
    level_ = static_cast<double>(capacity / 100);
    charging_ = (str_charging == "Discharging");

    udev_device_unref(dev);
    found = true;
    break;
  }

  udev_enumerate_unref(enumerate);

  if (!found) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Battery not found."));
  }

  return found;
}

gboolean SysInfoBattery::OnUpdateTimeout(gpointer user_data) {
  SysInfoBattery* instance = static_cast<SysInfoBattery*>(user_data);

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

    instance->PostMessageToListeners(output);
  }

  return TRUE;
}

void SysInfoBattery::SetData(picojson::value& data) {
  system_info::SetPicoJsonObjectValue(data, "level",
      picojson::value(level_));
  system_info::SetPicoJsonObjectValue(data, "isCharging",
      picojson::value(charging_));
}

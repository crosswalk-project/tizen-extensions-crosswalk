// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_battery.h"

#include <libudev.h>

#include "common/picojson.h"
#include "system_info/system_info_utils.h"

SysInfoBattery::SysInfoBattery(picojson::value& error) {
  picojson::object& error_map = error.get<picojson::object>();

  udev_ = udev_new();
  if (!udev_) {
    error_map["message"] = picojson::value("Can't create udev");
  }
}

SysInfoBattery::~SysInfoBattery() {
  if(udev_)
    udev_unref(udev_);
}

void SysInfoBattery::Update(picojson::value& error,
                            picojson::value& data) {
  picojson::object& error_map = error.get<picojson::object>();
  picojson::object& data_map = data.get<picojson::object>();
  struct udev_enumerate *enumerate;
  struct udev_list_entry *devices, *dev_list_entry;

  enumerate = udev_enumerate_new(udev_);
  udev_enumerate_add_match_subsystem(enumerate, "power_supply");
  udev_enumerate_scan_devices(enumerate);
  devices = udev_enumerate_get_list_entry(enumerate);

  error_map["message"] = picojson::value("Battery not found.");

  udev_list_entry_foreach(dev_list_entry, devices) {
    struct udev_device *dev;
    const char *path;
    std::string str_capacity, str_charging;

    path = udev_list_entry_get_name(dev_list_entry);
    dev = udev_device_new_from_syspath(udev_, path);

    str_capacity = system_info::get_udev_property(dev, "POWER_SUPPLY_CAPACITY");
    str_charging = system_info::get_udev_property(dev, "POWER_SUPPLY_STATUS");
    if (str_capacity.empty() && str_charging.empty()) {
      udev_device_unref(dev);
      continue;
    }

    // Found the battery
    int capacity = std::min(100, atoi(str_capacity.c_str()));
    data_map["level"] = picojson::value(static_cast<double>(capacity / 100));

    if (str_capacity == "Discharging")
      data_map["isCharging"] = picojson::value(false);
    else
      data_map["isCharging"] = picojson::value(false);
    error_map["message"] = picojson::value("");

    udev_device_unref(dev);
    break;
  }

  udev_enumerate_unref(enumerate);
}

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_STORAGE_H_
#define SYSTEM_INFO_SYSTEM_INFO_STORAGE_H_

#include <glib.h>
#include <libudev.h>

#include <map>
#include <string>

#include "common/picojson.h"
#include "common/utils.h"
#include "system_info/system_info_instance.h"
#include "system_info/system_info_utils.h"

enum StorageUnitType {
  UNKNOWN = 0,
  INTERNAL,
  USB_HOST,
  MMC
};

struct SysInfoDeviceStorageUnit {
  double available_capacity;
  double capacity;
  int id;
  bool is_removable;
  StorageUnitType type;
};

class SysInfoStorage : public SysInfoObject {
 public:
  static SysInfoObject& GetInstance() {
    static SysInfoStorage instance;
    return instance;
  }
  ~SysInfoStorage();
  void Get(picojson::value& error, picojson::value& data);
  void StartListening();
  void StopListening();

  static const std::string name_;

 private:
  SysInfoStorage();
  void GetAllAvailableStorageDevices();
  void InitStorageMonitor();
  void QueryAllAvailableStorageUnits();
  void MakeStorageUnit(SysInfoDeviceStorageUnit& unit, udev_device* dev) const;
  std::string ToStorageUnitTypeString(StorageUnitType type);
  void UpdateStorageList();
  static gboolean OnUpdateTimeout(gpointer user_data);

  int timeout_cb_id_;
  int udev_monitor_fd_;
  picojson::value units_;
  udev* udev_;
  udev_enumerate* enumerate_;
  udev_monitor* udev_monitor_;

  typedef std::map<int, SysInfoDeviceStorageUnit> StoragesMap;
  StoragesMap storages_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoStorage);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_STORAGE_H_

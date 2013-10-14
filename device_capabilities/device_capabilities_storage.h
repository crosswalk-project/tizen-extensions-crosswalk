// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CAPABILITIES_DEVICE_CAPABILITIES_STORAGE_H_
#define DEVICE_CAPABILITIES_DEVICE_CAPABILITIES_STORAGE_H_

#if defined(TIZEN_MOBILE)
#include <vconf.h>
#endif

#include <map>
#include <string>

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"
#include "device_capabilities/device_capabilities_utils.h"

struct DeviceStorageUnit {
  std::string id;
  std::string name;
  std::string type;
  double capacity;
};

class DeviceCapabilitiesStorage : public DeviceCapabilitiesObject {
 public:
  static DeviceCapabilitiesStorage& GetDeviceCapabilitiesStorage() {
    static DeviceCapabilitiesStorage instance;
    return instance;
  }
  ~DeviceCapabilitiesStorage() {}
  void Get(picojson::value& obj);
  void AddEventListener(std::string event_name,
                        DeviceCapabilitiesInstance* instance);
  void RemoveEventListener(DeviceCapabilitiesInstance* instance);

 private:
  explicit DeviceCapabilitiesStorage();

#if defined(TIZEN_MOBILE)
  void SetJsonValue(picojson::value& obj, const DeviceStorageUnit& unit);
  bool QueryInternal(DeviceStorageUnit& unit);
  bool QueryMMC(DeviceStorageUnit& unit);
  void UpdateStorageUnits(std::string command);
  static void OnStorageStatusChanged(keynode_t* node, void* user_data);
#endif

  typedef std::map<std::string, DeviceStorageUnit> StoragesMap;
  StoragesMap storages_;
};

#endif  // DEVICE_CAPABILITIES_DEVICE_CAPABILITIES_STORAGE_H_

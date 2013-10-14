// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CAPABILITIES_DEVICE_CAPABILITIES_MEMORY_H_
#define DEVICE_CAPABILITIES_DEVICE_CAPABILITIES_MEMORY_H_

#include <string>

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"
#include "device_capabilities/device_capabilities_utils.h"

class DeviceCapabilitiesMemory : public DeviceCapabilitiesObject {
 public:
  static DeviceCapabilitiesMemory& GetDeviceCapabilitiesMemory() {
    static DeviceCapabilitiesMemory instance;
    return instance;
  }
  ~DeviceCapabilitiesMemory() {}
  void Get(picojson::value& obj);
  void AddEventListener(std::string event_name,
                        DeviceCapabilitiesInstance* instance) { }
  void RemoveEventListener(DeviceCapabilitiesInstance* instance) { }

 private:
  explicit DeviceCapabilitiesMemory()
      : capacity_(0),
        availableCapacity_(0) { }

#if defined(TIZEN_MOBILE)
  bool QueryCapacity();
  bool QueryAvailableCapacity();
  void SetJsonValue(picojson::value& obj);
#endif

  unsigned int capacity_;
  unsigned int availableCapacity_;
};

#endif  // DEVICE_CAPABILITIES_DEVICE_CAPABILITIES_MEMORY_H_

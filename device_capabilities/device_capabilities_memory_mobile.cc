// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device_capabilities/device_capabilities_memory.h"

#include <device.h>

void DeviceCapabilitiesMemory::Get(picojson::value& obj) {
  if (QueryCapacity() && QueryAvailableCapacity()) {
    SetJsonValue(obj);
  }
}

void DeviceCapabilitiesMemory::SetJsonValue(picojson::value& obj) {
  picojson::object& o = obj.get<picojson::object>();
  o["capacity"] = picojson::value(static_cast<double>(capacity_));
  o["availableCapacity"] = picojson::value(
      static_cast<double>(availableCapacity_));
}

bool DeviceCapabilitiesMemory::QueryCapacity() {
  unsigned int capacity = 0;
  int ret = device_memory_get_total(&capacity);
  if (DEVICE_ERROR_NONE != ret) {
    return false;
  }

  capacity_ = capacity;
  return true;
}

bool DeviceCapabilitiesMemory::QueryAvailableCapacity() {
  unsigned int availablecapacity = 0;
  int ret = device_memory_get_available(&availablecapacity);
  if (DEVICE_ERROR_NONE != ret) {
    return false;
  }

  availableCapacity_ = availablecapacity;
  return true;
}

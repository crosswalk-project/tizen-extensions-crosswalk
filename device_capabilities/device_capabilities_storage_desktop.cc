// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device_capabilities/device_capabilities_storage.h"

DeviceCapabilitiesStorage::DeviceCapabilitiesStorage() { }

void DeviceCapabilitiesStorage::Get(picojson::value& obj) {
  // TODO(qjia7): This will be implemented later
}

void DeviceCapabilitiesStorage::AddEventListener(std::string event_name,
                                DeviceCapabilitiesInstance* instance) { }

void DeviceCapabilitiesStorage::RemoveEventListener(
    DeviceCapabilitiesInstance* instance) { }

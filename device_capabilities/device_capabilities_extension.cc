// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device_capabilities/device_capabilities_extension.h"

#include <string>
#include <utility>

#include "device_capabilities/device_capabilities_instance.h"

common::Extension* CreateExtension() {
  return new DeviceCapabilitiesExtension;
}

// This will be generated from device_capabilities_api.js.
extern const char kSource_device_capabilities_api[];

DeviceCapabilitiesExtension::DeviceCapabilitiesExtension() {
  SetExtensionName("navigator.system");
  SetJavaScriptAPI(kSource_device_capabilities_api);
  DeviceCapabilitiesInstance::DeviceInstanceMapInitialize();
}

DeviceCapabilitiesExtension::~DeviceCapabilitiesExtension() {}

common::Instance* DeviceCapabilitiesExtension::CreateInstance() {
  return new DeviceCapabilitiesInstance;
}

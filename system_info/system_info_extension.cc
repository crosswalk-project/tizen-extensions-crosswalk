// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <utility>

#include "system_info/system_info_extension.h"
#include "system_info/system_info_instance.h"

common::Extension* CreateExtension() {
  return new SystemInfoExtension;
}

// This will be generated from system_info_api.js.
extern const char kSource_system_info_api[];

SystemInfoExtension::SystemInfoExtension() : initialized_(false) {
  const char* entry_points[] = { NULL };
  SetExtraJSEntryPoints(entry_points);
  SetExtensionName("tizen.systeminfo");
  SetJavaScriptAPI(kSource_system_info_api);
}

SystemInfoExtension::~SystemInfoExtension() {}

common::Instance* SystemInfoExtension::CreateInstance() {
  if (!initialized_) {
    SystemInfoInstance::InstancesMapInitialize();
    initialized_ = true;
  }

  return new SystemInfoInstance;
}

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_setting/system_setting_extension.h"

#include "system_setting/system_setting_instance.h"

common::Extension* CreateExtension() {
  return new SystemSettingExtension;
}

// This will be generated from system_setting_api.js.
extern const char kSource_system_setting_api[];

SystemSettingExtension::SystemSettingExtension() {
  SetExtensionName("tizen.systemsetting");
  SetJavaScriptAPI(kSource_system_setting_api);
}

SystemSettingExtension::~SystemSettingExtension() {}

common::Instance* SystemSettingExtension::CreateInstance() {
  return new SystemSettingInstance;
}

// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "alarm/alarm_extension.h"

#include "alarm/alarm_instance.h"

common::Extension* CreateExtension() {
  return new AlarmExtension;
}

// This will be generated from alarm_api.js.
extern const char kSource_alarm_api[];

AlarmExtension::AlarmExtension() {
  SetExtensionName("tizen.alarm");
  SetJavaScriptAPI(kSource_alarm_api);
  const char* entry_points[] = {
    "tizen.AlarmAbsolute",
    "tizen.AlarmRelative",
    NULL
  };
  SetExtraJSEntryPoints(entry_points);
}

AlarmExtension::~AlarmExtension() {}

common::Instance* AlarmExtension::CreateInstance() {
  return new AlarmInstance();
}

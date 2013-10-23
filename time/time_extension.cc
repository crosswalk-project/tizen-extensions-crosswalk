// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "time/time_extension.h"

#include "time/time_instance.h"

common::Extension* CreateExtension() {
  return new TimeExtension;
}

// This will be generated from time_api.js.
extern const char kSource_time_api[];

TimeExtension::TimeExtension() {
  SetExtensionName("tizen.time");
  SetJavaScriptAPI(kSource_time_api);

  const char* entry_points[] = {
    "tizen.TZDate",
    "tizen.TimeDuration",
    NULL
  };
  SetExtraJSEntryPoints(entry_points);
}

TimeExtension::~TimeExtension() {}

common::Instance* TimeExtension::CreateInstance() {
  return new TimeInstance;
}

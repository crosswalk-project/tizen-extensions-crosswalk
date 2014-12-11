// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "power/power_extension.h"

#if defined(GENERIC_DESKTOP)
#include "power/power_instance_desktop.h"
#elif defined(TIZEN)
#include "power/power_instance_tizen.h"
#endif

common::Extension* CreateExtension() {
  return new PowerExtension;
}

// This will be generated from power_api.js.
extern const char kSource_power_api[];

PowerExtension::PowerExtension() {
  SetExtensionName("tizen.power");
  SetJavaScriptAPI(kSource_power_api);
}

PowerExtension::~PowerExtension() {}

common::Instance* PowerExtension::CreateInstance() {
#if defined(GENERIC_DESKTOP)
  return new PowerInstanceDesktop;
#elif defined(TIZEN)
  return new PowerInstanceMobile(&power_event_source_);
#endif
  return NULL;
}

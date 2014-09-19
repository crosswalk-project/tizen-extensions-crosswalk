// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "telephony/telephony_extension.h"

#include <glib-object.h>
#include "telephony/telephony_instance.h"

common::Extension* CreateExtension() {
  return new TelephonyExtension;
}

extern const char kSource_telephony_api[];

TelephonyExtension::TelephonyExtension() {
  SetExtensionName("tizen.telephony");
  SetJavaScriptAPI(kSource_telephony_api);
}

TelephonyExtension::~TelephonyExtension() {}

common::Instance* TelephonyExtension::CreateInstance() {
  return new TelephonyInstance;
}

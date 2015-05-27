// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "phone/phone_extension.h"
#include "phone/phone_instance.h"

common::Extension* CreateExtension() {
  return new PhoneExtension();
}

// This will be generated from phone_api.js
extern const char kSource_phone_api[];

PhoneExtension::PhoneExtension() {
  SetExtensionName("tizen.phone");
  SetJavaScriptAPI(kSource_phone_api);
}

PhoneExtension::~PhoneExtension() {}

common::Instance* PhoneExtension::CreateInstance() {
  return new PhoneInstance;
}

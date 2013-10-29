// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "messageport/messageport_extension.h"

#include "messageport/messageport_instance.h"

common::Extension* CreateExtension() {
  return new MessageportExtension;
}

// This will be generated from messageport_api.js.
extern const char kSource_messageport_api[];

MessageportExtension::MessageportExtension() {
  SetExtensionName("tizen.messageport");
  SetJavaScriptAPI(kSource_messageport_api);
}

common::Instance* MessageportExtension::CreateInstance() {
  return new MessageportInstance();
}

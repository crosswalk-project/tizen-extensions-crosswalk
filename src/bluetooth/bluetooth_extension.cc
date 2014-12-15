// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bluetooth/bluetooth_extension.h"

#if defined(TIZEN_CAPI_BT)
#include "bluetooth/bluetooth_instance_capi.h"
#else
#include "bluetooth/bluetooth_instance.h"
#endif

common::Extension* CreateExtension() {
  return new BluetoothExtension;
}

// This will be generated from bluetooth_api.js.
extern const char kSource_bluetooth_api[];

BluetoothExtension::BluetoothExtension() {
  const char* entry_points[] = { NULL };
  SetExtraJSEntryPoints(entry_points);
  SetExtensionName("tizen.bluetooth");
  SetJavaScriptAPI(kSource_bluetooth_api);
}

BluetoothExtension::~BluetoothExtension() {}

common::Instance* BluetoothExtension::CreateInstance() {
  return new BluetoothInstance;
}

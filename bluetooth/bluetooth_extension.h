// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BLUETOOTH_BLUETOOTH_EXTENSION_H_
#define BLUETOOTH_BLUETOOTH_EXTENSION_H_

#include "common/extension.h"

class BluetoothExtension : public common::Extension {
 public:
  BluetoothExtension();
  virtual ~BluetoothExtension();

 private:
  // common::Extension implementation.
  virtual common::Instance* CreateInstance();
};

#endif  // BLUETOOTH_BLUETOOTH_EXTENSION_H_

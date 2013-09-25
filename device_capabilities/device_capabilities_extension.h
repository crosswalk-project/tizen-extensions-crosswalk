// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CAPABILITIES_DEVICE_CAPABILITIES_EXTENSION_H_
#define DEVICE_CAPABILITIES_DEVICE_CAPABILITIES_EXTENSION_H_

#include "common/extension.h"

class DeviceCapabilitiesExtension : public common::Extension {
 public:
  DeviceCapabilitiesExtension();
  virtual ~DeviceCapabilitiesExtension();

 private:
  // common::Extension implementation.
  virtual common::Instance* CreateInstance();
};

#endif  // DEVICE_CAPABILITIES_DEVICE_CAPABILITIES_EXTENSION_H_

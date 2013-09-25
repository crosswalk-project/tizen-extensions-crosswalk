// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CAPABILITIES_DEVICE_CAPABILITIES_INSTANCE_H_
#define DEVICE_CAPABILITIES_DEVICE_CAPABILITIES_INSTANCE_H_

#include <string>

#include "common/extension.h"
#include "common/picojson.h"

namespace picojson {
class value;
}

class DeviceCapabilitiesInstance : public common::Instance {
 public:
  explicit DeviceCapabilitiesInstance() { }
  virtual ~DeviceCapabilitiesInstance();
  static void DeviceInstanceMapInitialize();

 private:
  // common::Instance implementation.
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* msg);

  void HandleGetDeviceInfo(std::string deviceName, const picojson::value& msg);
  void HandleAddEventListener(const picojson::value& msg);
};

#endif  // DEVICE_CAPABILITIES_DEVICE_CAPABILITIES_INSTANCE_H_

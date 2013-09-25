// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CAPABILITIES_DEVICE_CAPABILITIES_UTILS_H_
#define DEVICE_CAPABILITIES_DEVICE_CAPABILITIES_UTILS_H_

#include <list>
#include <map>
#include <string>

#include "common/picojson.h"
#include "device_capabilities/device_capabilities_instance.h"

typedef std::list<DeviceCapabilitiesInstance*> DeviceCapabilitiesEventsList;
static DeviceCapabilitiesEventsList device_storage_attach_events_;
static DeviceCapabilitiesEventsList device_storage_detach_events_;

class DeviceCapabilitiesObject {
 public:
  virtual void Get(picojson::value& v) = 0;
  virtual void AddEventListener(std::string event_name,
                                DeviceCapabilitiesInstance* instance) = 0;
  virtual void RemoveEventListener(DeviceCapabilitiesInstance* instance) = 0;
};

static std::map<std::string, DeviceCapabilitiesObject&> device_instatances_map_;

#endif  // DEVICE_CAPABILITIES_DEVICE_CAPABILITIES_UTILS_H_

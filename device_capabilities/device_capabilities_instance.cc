// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device_capabilities/device_capabilities_instance.h"

#include <utility>
#include <map>

#include "device_capabilities/device_capabilities_cpu.h"
#include "device_capabilities/device_capabilities_memory.h"
#include "device_capabilities/device_capabilities_storage.h"
#include "device_capabilities/device_capabilities_utils.h"

std::map<std::string, DeviceCapabilitiesObject&>::iterator device_instance_;

DeviceCapabilitiesInstance::~DeviceCapabilitiesInstance() {
  for (device_instance_ = device_instatances_map_.begin();
       device_instance_ != device_instatances_map_.end();
       device_instance_++) {
    (device_instance_->second).RemoveEventListener(this);
  }
}

void DeviceCapabilitiesInstance::DeviceInstanceMapInitialize() {
  device_instatances_map_.insert(
      std::pair<std::string, DeviceCapabilitiesObject&>(
      "CPU", DeviceCapabilitiesCpu::GetDeviceCapabilitiesCpu()));
  device_instatances_map_.insert(
      std::pair<std::string, DeviceCapabilitiesObject&>(
      "Memory", DeviceCapabilitiesMemory::GetDeviceCapabilitiesMemory()));
  device_instatances_map_.insert(
      std::pair<std::string, DeviceCapabilitiesObject&>(
      "Storage", DeviceCapabilitiesStorage::GetDeviceCapabilitiesStorage()));
}

void DeviceCapabilitiesInstance::HandleSyncMessage(const char* message) {
}

void DeviceCapabilitiesInstance::HandleMessage(const char* message) {
  picojson::value v;
  std::string err;

  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }

  std::string cmd = v.get("cmd").to_str();

  if (cmd == "getCPUInfo") {
    HandleGetDeviceInfo("CPU", v);
  } else if (cmd == "getMemoryInfo") {
    HandleGetDeviceInfo("Memory", v);
  } else if (cmd == "getStorageInfo") {
    HandleGetDeviceInfo("Storage", v);
  } else if (cmd == "getDisplayInfo") {
    HandleGetDeviceInfo("Display", v);
  } else if (cmd == "addEventListener") {
    HandleAddEventListener(v);
  }
}

void
DeviceCapabilitiesInstance::HandleGetDeviceInfo(std::string deviceName,
                                                const picojson::value& msg) {
  std::string reply_id = msg.get("_reply_id").to_str();
  picojson::value output = picojson::value(picojson::object());
  picojson::object& o = output.get<picojson::object>();
  o["_reply_id"] = picojson::value(reply_id);

  device_instance_ = device_instatances_map_.find(deviceName);
  if (device_instance_ != device_instatances_map_.end()) {
    (device_instance_->second).Get(output);
  }

  PostMessage(output.serialize().c_str());
}

void
DeviceCapabilitiesInstance::HandleAddEventListener(const picojson::value& msg) {
  std::string event_name = msg.get("eventName").to_str();

  if (event_name == "onattach" || event_name == "ondetach") {
    device_instance_ = device_instatances_map_.find("Storage");
    if (device_instance_ != device_instatances_map_.end()) {
      (device_instance_->second).AddEventListener(event_name, this);
    }
  } else if (event_name == "onconnect" || event_name == "ondisconnect") {
    device_instance_ = device_instatances_map_.find("Display");
    if (device_instance_ != device_instatances_map_.end()) {
      (device_instance_->second).AddEventListener(event_name, this);
    }
  }
}

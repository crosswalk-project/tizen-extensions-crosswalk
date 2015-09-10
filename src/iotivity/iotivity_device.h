/*
 * Copyright (c) 2015 Cisco and/or its affiliates. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Cisco nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef IOTIVITY_IOTIVITY_DEVICE_H_
#define IOTIVITY_IOTIVITY_DEVICE_H_

#include <map>
#include <string>
#include "iotivity/iotivity_tools.h"
#include "common/extension.h"

namespace common {
class Instance;
}

class IotivityServer;
class IotivityClient;

class IotivityDeviceInfo {
 public:
  std::map<std::string, std::string> m_deviceinfomap;

 public:
  IotivityDeviceInfo();
  ~IotivityDeviceInfo();

  int mapSize();
  std::string hasMap(std::string key);
  void deserialize(const picojson::value& value);
  void serialize(picojson::object& object);
};

class IotivityDeviceSettings {
 public:
  std::string m_url;
  IotivityDeviceInfo m_deviceInfo;
  std::string m_role;
  std::string m_connectionMode;

 public:
  IotivityDeviceSettings();
  ~IotivityDeviceSettings();

  void deserialize(const picojson::value& value);
};

class IotivityDevice {
 private:
  common::Instance* m_instance;
  IotivityServer* m_server;
  IotivityClient* m_client;

 public:
  explicit IotivityDevice(common::Instance* instance);
  explicit IotivityDevice(common::Instance* instance,
                          IotivityDeviceSettings* settings);
  ~IotivityDevice();

  common::Instance* getInstance();
  IotivityServer* getServer();
  IotivityClient* getClient();

  void configure(IotivityDeviceSettings* settings);
  OCStackResult configurePlatformInfo(IotivityDeviceInfo& deviceInfo);
  OCStackResult configureDeviceInfo(IotivityDeviceInfo& deviceInfo);

  void handleConfigure(const picojson::value& value);
  void handleFactoryReset(const picojson::value& value);
  void handleReboot(const picojson::value& value);
  void PostMessage(const char* msg);
  void postResult(const char* completed_operation, double async_operation_id);
  void postError(double async_operation_id);
};

#endif  // IOTIVITY_IOTIVITY_DEVICE_H_

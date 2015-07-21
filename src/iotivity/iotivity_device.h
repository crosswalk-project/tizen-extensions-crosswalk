/****************************************************************************
**
** Copyright © 1992-2014 Cisco and/or its affiliates. All rights reserved.
** All rights reserved.
**
** $CISCO_BEGIN_LICENSE:APACHE$
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
** http://www.apache.org/licenses/LICENSE-2.0
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
** $CISCO_END_LICENSE$
**
****************************************************************************/
#ifndef IOTIVITY_IOTIVITY_DEVICE_H_
#define IOTIVITY_IOTIVITY_DEVICE_H_

#include <string>
#include "iotivity/iotivity_tools.h"
#include "common/extension.h"


namespace common {
class Instance;
}

class IotivityServer;
class IotivityClient;


class IotivityDevice {
 private:
  common::Instance* m_instance;
  IotivityServer* m_server;
  IotivityClient* m_client;

 public:
  explicit IotivityDevice(common::Instance* instance);
  ~IotivityDevice();

  common::Instance* getInstance();
  IotivityServer* getServer();
  IotivityClient* getClient();

  void handleConfigure(const picojson::value& value);
  void handleFactoryReset(const picojson::value& value);
  void handleReboot(const picojson::value& value);
  void PostMessage(const char *msg);
  void postResult(const char* completed_operation, double async_operation_id);
  void postError(double async_operation_id);
};

#endif  // IOTIVITY_IOTIVITY_DEVICE_H_

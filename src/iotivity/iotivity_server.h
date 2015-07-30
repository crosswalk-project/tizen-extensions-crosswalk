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
#ifndef IOTIVITY_IOTIVITY_SERVER_H_
#define IOTIVITY_IOTIVITY_SERVER_H_

#include <map>
#include <string>
#include "iotivity/iotivity_tools.h"

class IotivityDevice;

class IotivityServer {
 private:
  IotivityDevice* m_device;
  std::map<std::string, void *> m_resourcemap;

 public:
  explicit IotivityServer(IotivityDevice* device);
  ~IotivityServer();

  void *getResourceById(std::string id);
  void handleRegisterResource(const picojson::value& value);
  void handleUnregisterResource(const picojson::value& value);
  void handleEnablePresence(const picojson::value& value);
  void handleDisablePresence(const picojson::value& value);
  void handleNotify(const picojson::value& value);
};

#endif  // IOTIVITY_IOTIVITY_SERVER_H_

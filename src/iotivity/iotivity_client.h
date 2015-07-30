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
#ifndef IOTIVITY_IOTIVITY_CLIENT_H_
#define IOTIVITY_IOTIVITY_CLIENT_H_

#include <map>
#include <string>
#include "iotivity/iotivity_tools.h"

class IotivityDevice;

class IotivityClient {
 private:
  IotivityDevice* m_device;
  // Map resourceId fullpath with pointer
  std::map<std::string, void *> m_resourcemap;
  std::map<std::string, void *> m_foundresourcemap;
  std::mutex m_callbackLock;

  // Map device UUID with pointer
  std::map<std::string, void *> m_devicemap;
  std::map<std::string, void *> m_founddevicemap;
  std::mutex m_callbackLockDevices;

  std::string m_discoveryOptionDeviceId;


 public:
  explicit IotivityClient(IotivityDevice* device);
  ~IotivityClient();

  double m_asyncCallId_findresources;
  double m_asyncCallId_finddevices;

  void *getResourceById(std::string id);

  void foundResourceCallback(std::shared_ptr<OCResource> resource, int waitsec);
  void handleFindResources(const picojson::value& value);
  void findResourceTimerCallback(int waitsec);
  void findPreparedRequest(void);

  void foundDeviceCallback(const OCRepresentation& rep, int waitsec);
  void handleFindDevices(const picojson::value& value);
  void findDeviceTimerCallback(int waitsec);
  void findDevicePreparedRequest(void);

  void handleCreateResource(const picojson::value& value);
  void handleRetrieveResource(const picojson::value& value);
  void handleUpdateResource(const picojson::value& value);
  void handleDeleteResource(const picojson::value& value);
  void handleStartObserving(const picojson::value& value);
  void handleCancelObserving(const picojson::value& value);
};

#endif  // IOTIVITY_IOTIVITY_CLIENT_H_


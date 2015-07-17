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
#ifndef IOTIVITY_CLIENT_H_
#define IOTIVITY_CLIENT_H_

#include "iotivity/iotivity_tools.h"

class IotivityDevice;

class IotivityClient {

 private:
  IotivityDevice* m_device;
  std::map<int, void *> m_resourcemap;

 public:
  IotivityClient(IotivityDevice* device);
  ~IotivityClient();

  double m_asyncCallId_findresources;
  double m_asyncCallId_finddevice;

  void *getResourceById(int id);
  void foundResourceCallback(std::shared_ptr<OCResource> resource);
  void handleFindResources(const picojson::value& value);

  void foundDeviceCallback(const OCRepresentation& rep);
  void handleFindDevices(const picojson::value& value);

  
  void handleCreateResource(const picojson::value& value);
  void handleRetrieveResource(const picojson::value& value);
  void handleUpdateResource(const picojson::value& value);
  void handleDeleteResource(const picojson::value& value);
  void handleStartObserving(const picojson::value& value);
  void handleCancelObserving(const picojson::value& value);



};

#endif  // IOTIVITY_CLIENT_H_


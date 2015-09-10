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
#ifndef IOTIVITY_IOTIVITY_CLIENT_H_
#define IOTIVITY_IOTIVITY_CLIENT_H_

#include <map>
#include <string>
#include "iotivity/iotivity_tools.h"
#include "iotivity/iotivity_resource.h"

class IotivityDevice;

class IotivityClient {
 private:
  IotivityDevice* m_device;
  // Map resourceId fullpath with pointer
  std::map<std::string, IotivityResourceClient*> m_resourcemap;
  std::map<std::string, IotivityResourceClient*> m_foundresourcemap;
  std::mutex m_callbackLock;

  // Map device UUID with pointer
  std::map<std::string, IotivityDeviceInfo*> m_devicemap;
  std::map<std::string, IotivityDeviceInfo*> m_founddevicemap;
  std::mutex m_callbackLockDevices;

  std::string m_discoveryOptionDeviceId;

 public:
  explicit IotivityClient(IotivityDevice* device);
  ~IotivityClient();

  double m_asyncCallId_findresources;
  double m_asyncCallId_finddevices;

  IotivityResourceClient* getResourceById(std::string id);

  void foundResourceCallback(std::shared_ptr<OCResource> resource, int waitsec);
  void handleFindResources(const picojson::value& value);
  void findResourceTimerCallback(int waitsec);
  void findPreparedRequest(void);

  void foundPlatformCallback(const OCRepresentation& rep);
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

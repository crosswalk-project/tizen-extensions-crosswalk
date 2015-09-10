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
#ifndef IOTIVITY_IOTIVITY_RESOURCE_H_
#define IOTIVITY_IOTIVITY_RESOURCE_H_

#include <string>
#include <vector>
#include "iotivity/iotivity_tools.h"
#include "iotivity/iotivity_device.h"

namespace common {
class Instance;
}

// Map on JS OicResourceInit
class IotivityResourceInit {
 public:
  std::string m_url;
  std::string m_deviceId;
  std::string m_connectionMode;

  bool m_discoverable;
  bool m_observable;
  bool m_isSecure;

  std::vector<std::string> m_resourceTypeNameArray;
  std::string m_resourceTypeName;
  std::vector<std::string> m_resourceInterfaceArray;
  std::string m_resourceInterface;

  uint8_t m_resourceProperty;

  OCRepresentation m_resourceRep;

 public:
  explicit IotivityResourceInit(const picojson::value& value);
  IotivityResourceInit();
  ~IotivityResourceInit();

  void deserialize(const picojson::value& value);
  void serialize(picojson::object& object);
};

// Map on JS OicResource
class IotivityResourceServer {
 private:
  IotivityDevice* m_device;
  IotivityResourceInit* m_oicResourceInit;
  OCResourceHandle m_resourceHandle;
  ObservationIds m_interestedObservers;
  std::string m_idfull;

 public:
  IotivityResourceServer(IotivityDevice* device,
                         IotivityResourceInit* oicResource);
  ~IotivityResourceServer();

  OCEntityHandlerResult entityHandlerCallback(
      std::shared_ptr<OCResourceRequest> request);
  OCStackResult registerResource();

  int getResourceHandleToInt();
  std::string getResourceId();
  OCRepresentation getRepresentation();
  ObservationIds& getObserversList();
  void serialize(picojson::object& object);
};

// Map on JS OicResource
class IotivityResourceClient {
 private:
  IotivityDevice* m_device;

  shared_ptr<OCResource> m_ocResourcePtr;
  IotivityResourceInit* m_oicResourceInit;

  int m_id;
  std::string m_idfull;
  std::string m_sid;
  std::string m_host;

 public:
  explicit IotivityResourceClient(IotivityDevice* device);
  ~IotivityResourceClient();

  void setSharedPtr(std::shared_ptr<OCResource> sharePtr);
  int getResourceHandleToInt();
  std::string getResourceId();
  void serialize(picojson::object& object);

  void onPut(const HeaderOptions& headerOptions, const OCRepresentation& rep,
             const int eCode, double asyncCallId);
  void onGet(const HeaderOptions& headerOptions, const OCRepresentation& rep,
             const int eCode, double asyncCallId);
  void onPost(const HeaderOptions& headerOptions, const OCRepresentation& rep,
              const int eCode, double asyncCallId);
  void onStartObserving(double asyncCallId);
  void onObserve(const HeaderOptions headerOptions, const OCRepresentation& rep,
                 const int& eCode, const int& sequenceNumber,
                 double asyncCallId);
  void onDelete(const HeaderOptions& headerOptions, const int eCode,
                double asyncCallId);

  OCStackResult createResource(IotivityResourceInit& oicResourceInit,
                               double asyncCallId);
  OCStackResult retrieveResource(double asyncCallId);
  OCStackResult updateResource(OCRepresentation& representation,
                               double asyncCallId);
  OCStackResult deleteResource(double asyncCallId);
  OCStackResult startObserving(double asyncCallId);
  OCStackResult cancelObserving(double asyncCallId);
};

// Map on JS OicRequestEvent
class IotivityRequestEvent {
 public:
  IotivityDevice* m_device;

  std::string m_type;
  int m_requestId;
  std::string m_source;
  std::string m_target;

  OCRepresentation m_resourceRep;
  std::vector<std::string> m_updatedPropertyNames;

  QueryParamsMap m_queries;
  HeaderOptions m_headerOptions;

  OCRepresentation m_resourceRepTarget;

 public:
  IotivityRequestEvent();
  ~IotivityRequestEvent();

  void deserialize(std::shared_ptr<OCResourceRequest> request);
  void deserialize(const picojson::value& value);
  void serialize(picojson::object& object);

  OCStackResult sendResponse();
  OCStackResult sendError();
};

#endif  // IOTIVITY_IOTIVITY_RESOURCE_H_

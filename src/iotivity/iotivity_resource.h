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
#ifndef IOTIVITY_RESOURCE_H_
#define IOTIVITY_RESOURCE_H_


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
  IotivityResourceInit(const picojson::value& value);
  IotivityResourceInit();
  ~IotivityResourceInit();

  void deserialize(const picojson::value& value);
  void serialize(picojson::object& object);
};


// Map on JS OicResource
class IotivityResourceServer {

 private:
  //shared_ptr<OCResource> m_ocResourcePtr;
  IotivityDevice *m_device;
  IotivityResourceInit *m_oicResourceInit;

  OCResourceHandle m_resourceHandle;
  ObservationIds m_interestedObservers;


 public:
  IotivityResourceServer(IotivityDevice *device, IotivityResourceInit *oicResource);
  ~IotivityResourceServer();

  OCEntityHandlerResult entityHandlerCallback(std::shared_ptr<OCResourceRequest> request);
  OCStackResult registerResource();

  int getResourceHandleToInt();
  void serialize(picojson::object& object);

};

// Map on JS OicResource
class IotivityResourceClient {

 private:
  IotivityDevice *m_device;

  shared_ptr<OCResource> m_ocResourcePtr;
  IotivityResourceInit *m_oicResourceInit;

  int m_id;
  std::string m_sid;
  std::string m_host;

 public:
  IotivityResourceClient(IotivityDevice *device);
  ~IotivityResourceClient();

  double m_asyncCallId_create;
  double m_asyncCallId_retrieve;
  double m_asyncCallId_update;
  double m_asyncCallId_delete;
  double m_asyncCallId_observe;


  void setSharedPtr(std::shared_ptr<OCResource> sharePtr);
  int getResourceHandleToInt();
  void serialize(picojson::object& object);


  void onPut(const HeaderOptions& headerOptions, const OCRepresentation& rep, const int eCode);
  void onGet(const HeaderOptions& headerOptions, const OCRepresentation& rep, const int eCode);
  void onPost(const HeaderOptions& headerOptions, const OCRepresentation& rep, const int eCode);
  void onObserve(const HeaderOptions headerOptions, const OCRepresentation& rep,
                 const int& eCode, const int& sequenceNumber);
  void onDelete(const HeaderOptions& headerOptions, const int eCode);

  OCStackResult createResource(IotivityResourceInit & oicResourceInit);
  OCStackResult retrieveResource();
  OCStackResult updateResource(OCRepresentation & representation);
  OCStackResult deleteResource();
  OCStackResult startObserving();
  OCStackResult cancelObserving();
};


// Map on JS OicRequestEvent
class IotivityRequestEvent {

 public:
  IotivityDevice *m_device;

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


#endif  // IOTIVITY_RESOURCE_H_


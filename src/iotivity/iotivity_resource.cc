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
#include "iotivity/iotivity_resource.h"
#include "common/extension.h"

IotivityResourceInit::IotivityResourceInit() {
    m_url = "";
    m_deviceId = "";
    m_connectionMode = "";
    m_discoverable = false;
    m_observable = false;
    m_isSecure = false;
}

IotivityResourceInit::IotivityResourceInit(const picojson::value& value) {
    m_url = "";
    m_deviceId = "";
    m_connectionMode = "";
    m_discoverable = false;
    m_observable = false;
    m_isSecure = false;

    deserialize(value);
}

IotivityResourceInit::~IotivityResourceInit() {}

void IotivityResourceInit::deserialize(const picojson::value& value) {
    DEBUG_MSG("IotivityResourceInit::deserialize\n");

    m_url = value.get("url").to_str();
    m_deviceId = value.get("deviceId").to_str();
    m_connectionMode = value.get("connectionMode").to_str();
    m_discoverable = value.get("discoverable").get<bool>();
    m_observable = value.get("observable").get<bool>();
    m_isSecure = false;
    m_resourceTypeName = "";
    m_resourceInterface = "";
    m_resourceProperty = 0;

    picojson::array resourceTypes =
      value.get("resourceTypes").get<picojson::array>();

    for (picojson::array::iterator iter = resourceTypes.begin();
      iter != resourceTypes.end(); ++iter) {
        DEBUG_MSG("array resourceTypes value=%s\n",
          (*iter).get<string>().c_str());
        if (m_resourceTypeName == "") {
            m_resourceTypeName = (*iter).get<string>();
        }

        m_resourceTypeNameArray.push_back((*iter).get<string>());
    }

    picojson::array interfaces = value.get("interfaces").get<picojson::array>();

    for (picojson::array::iterator iter = interfaces.begin();
      iter != interfaces.end(); ++iter) {
        DEBUG_MSG("array interfaces value=%s\n", (*iter).get<string>().c_str());
        if (m_resourceInterface == "") {
            m_resourceInterface = (*iter).get<string>();
        }
        m_resourceInterfaceArray.push_back((*iter).get<string>());
    }

    if (m_resourceInterface == "")
        m_resourceInterface = DEFAULT_INTERFACE;

    if (m_discoverable)
        m_resourceProperty |= OC_DISCOVERABLE;

    if (m_observable)
        m_resourceProperty |= OC_OBSERVABLE;

    if (m_isSecure)
        m_resourceProperty |= OC_SECURE;

    DEBUG_MSG("discoverable=%d, observable=%d, isSecure=%d\n", m_discoverable,
      m_observable, m_isSecure);
    DEBUG_MSG("SVR: uri=%s, type=%s, itf=%s, prop=%d\n",
              m_url.c_str(),
              m_resourceTypeName.c_str(),
              m_resourceInterface.c_str(),
              m_resourceProperty);

    picojson::value properties = value.get("properties");
    picojson::object & propertiesobject = properties.get<picojson::object>();
    DEBUG_MSG("properties: size=%d\n", propertiesobject.size());

    for (picojson::value::object::iterator iter = propertiesobject.begin();
            iter != propertiesobject.end();
            ++iter) {
        std::string objectKey = iter->first;
        picojson::value objectValue = iter->second;

        if (objectValue.is<bool>()) {
            DEBUG_MSG("[bool] key=%s, value=%d\n", objectKey.c_str(),
              objectValue.get<bool>());
            m_resourceRep[objectKey] = objectValue.get<bool>();
        } else if (objectValue.is<double>()) {
            DEBUG_MSG("[double] key=%s, value=%f\n", objectKey.c_str(),
              objectValue.get<double>());
            m_resourceRep[objectKey] = objectValue.get<double>();
        } else if (objectValue.is<int>()) {
            DEBUG_MSG("[string] key=%s, value=%d\n", objectKey.c_str(),
              objectValue.get<int>());
            m_resourceRep[objectKey] = objectValue.get<int>();
        } else if (objectValue.is<string>()) {
            DEBUG_MSG("[string] key=%s, value=%s\n", objectKey.c_str(),
              objectValue.get<string>().c_str());
            m_resourceRep[objectKey] = objectValue.get<string>();
        }
    }
}

void IotivityResourceInit::serialize(picojson::object& object) {
    object["url"] = picojson::value(m_url);
    object["deviceId"] = picojson::value(m_deviceId);
    object["connectionMode"] = picojson::value(m_connectionMode);
    object["discoverable"] = picojson::value(m_discoverable);
    object["observable"] = picojson::value(m_observable);
    picojson::array resourceTypes;

    CopyInto(m_resourceTypeNameArray, resourceTypes);

    object["resourceTypes"] = picojson::value(resourceTypes);
    picojson::array interfaces;

    CopyInto(m_resourceInterfaceArray, interfaces);

    object["interfaces"] = picojson::value(interfaces);
    picojson::object properties;
    TranslateOCRepresentationToPicojson(m_resourceRep, properties);
    object["properties"] = picojson::value(properties);
}

IotivityResourceServer::IotivityResourceServer(IotivityDevice *device,
  IotivityResourceInit *oicResource) : m_device(device) {
    m_oicResourceInit = oicResource;
    m_resourceHandle = 0;
}

IotivityResourceServer::~IotivityResourceServer() {
    delete m_oicResourceInit;

    if (m_resourceHandle != 0) {
        OCPlatform::unregisterResource(m_resourceHandle);
        m_resourceHandle = 0;
    }

    if (m_oicResourceInit) {
        delete m_oicResourceInit;
        m_oicResourceInit = NULL;
    }
}

OCRepresentation IotivityResourceServer::getRepresentation() {
    return m_oicResourceInit->m_resourceRep;
}

ObservationIds & IotivityResourceServer::getObserversList() {
    return m_interestedObservers;
}

OCEntityHandlerResult IotivityResourceServer::entityHandlerCallback(
  std::shared_ptr<OCResourceRequest> request) {
    DEBUG_MSG("\n\n[Remote Client==>] entityHandlerCallback:\n");

    OCEntityHandlerResult ehResult = OC_EH_ERROR;
    picojson::value::object object;

    if (request) {
        ehResult = OC_EH_OK;
        IotivityRequestEvent iotivityRequestEvent;
        iotivityRequestEvent.deserialize(request);
        iotivityRequestEvent.m_resourceRepTarget =
            m_oicResourceInit->m_resourceRep;
        int requestFlag = request->getRequestHandlerFlag();

        if (requestFlag & RequestHandlerFlag::ObserverFlag) {
            DEBUG_MSG("postEntityHandler:ObserverFlag\n");

            iotivityRequestEvent.m_type = "observe";
            ObservationInfo observationInfo = request->getObservationInfo();

            if (ObserveAction::ObserveRegister == observationInfo.action) {
                DEBUG_MSG("postEntityHandler:ObserveRegister\n");
                m_interestedObservers.push_back(observationInfo.obsId);
            } else if (ObserveAction::ObserveUnregister ==
                observationInfo.action) {
                DEBUG_MSG("postEntityHandler:ObserveUnregister\n");
                m_interestedObservers.erase(
                               std::remove(m_interestedObservers.begin(),
                               m_interestedObservers.end(),
                               observationInfo.obsId),
                               m_interestedObservers.end());
            }
        }

        if (iotivityRequestEvent.m_type == "update") {
            UpdateOcRepresentation(iotivityRequestEvent.m_resourceRep,
                    m_oicResourceInit->m_resourceRep,
                    iotivityRequestEvent.m_updatedPropertyNames);
        }

        object["cmd"] = picojson::value("entityHandler");
        picojson::object OicRequestEvent;
        iotivityRequestEvent.serialize(OicRequestEvent);
        object["OicRequestEvent"] = picojson::value(OicRequestEvent);
        picojson::value value(object);
        m_device->PostMessage(value.serialize().c_str());
    } else {
        ERROR_MSG("entityHandlerCallback: Request invalid");
    }

    return ehResult;
}

void IotivityResourceServer::serialize(picojson::object& object) {
    object["id"] = picojson::value(m_idfull);
    picojson::object properties;
    m_oicResourceInit->serialize(properties);
    object["OicResourceInit"] = picojson::value(properties);
}

OCStackResult IotivityResourceServer::registerResource() {
    OCStackResult result = OC_STACK_ERROR;
    EntityHandler resourceCallback =
        std::bind(&IotivityResourceServer::entityHandlerCallback,
                  this,
                  std::placeholders::_1);

    result = OCPlatform::registerResource(
        m_resourceHandle,
        m_oicResourceInit->m_url,
        m_oicResourceInit->m_resourceTypeName,
        m_oicResourceInit->m_resourceInterface,
        resourceCallback,
        m_oicResourceInit->m_resourceProperty);

    if (OC_STACK_OK != result) {
        ERROR_MSG("registerResource was unsuccessful\n");
        return result;
    }

    DEBUG_MSG("registerResource handle=%d\n", m_resourceHandle);

    // TODO(aphao) should retrieve host IP + url (cleaner) ??
    // + m_oicResourceInit->m_url;
    m_idfull = std::to_string(getResourceHandleToInt());

    if (m_oicResourceInit->m_resourceTypeNameArray.size() >= 2) {
        for (int i = 1;
          i < m_oicResourceInit->m_resourceTypeNameArray.size(); i++) {
            std::string resourceTypeName =
              m_oicResourceInit->m_resourceTypeNameArray[i];

            DEBUG_MSG("bindTypeToResource=%s\n", resourceTypeName.c_str());

            result = OCPlatform::bindTypeToResource(m_resourceHandle,
                                                    resourceTypeName);

            if (OC_STACK_OK != result) {
                ERROR_MSG("bindTypeToResource TypeName "
                          "to Resource unsuccessful\n");
                return result;
            }
        }
    }

    int iSize = m_oicResourceInit->m_resourceInterfaceArray.size();
    if (iSize >= 2) {
        for (int i = 1; i < iSize; i++) {
            std::string resourceInterface =
              m_oicResourceInit->m_resourceInterfaceArray[i];

            DEBUG_MSG("bindInterfaceToResource=%s\n",
              resourceInterface.c_str());

            result = OCPlatform::bindInterfaceToResource(m_resourceHandle,
                                                         resourceInterface);

            if (OC_STACK_OK != result) {
                ERROR_MSG("Binding InterfaceName to Resource unsuccessful\n");
                return result;
            }
        }
    }

    return result;
}

int IotivityResourceServer::getResourceHandleToInt() {
    int *p = reinterpret_cast<int *>(m_resourceHandle);
    int pint = reinterpret_cast<int>(p);
    return pint;
}

std::string IotivityResourceServer::getResourceId() {
    return m_idfull;
}

IotivityResourceClient::IotivityResourceClient(IotivityDevice *device) :
  m_device(device) {
    m_ocResourcePtr = NULL;
    m_oicResourceInit = new IotivityResourceInit();
}

IotivityResourceClient::~IotivityResourceClient() {
    if (m_oicResourceInit) {
        delete m_oicResourceInit;
        m_oicResourceInit = NULL;
    }
}

void IotivityResourceClient::setSharedPtr(
  std::shared_ptr<OCResource> sharePtr) {
    m_ocResourcePtr = sharePtr;
    int *p = reinterpret_cast<int *>(sharePtr.get());
    int pint = reinterpret_cast<int>(p);
    m_id = pint;
    m_oicResourceInit->m_url = sharePtr->uri();
    m_oicResourceInit->m_deviceId = sharePtr->sid();
    m_oicResourceInit->m_connectionMode = "default";
    // TODO(aphao) : m_discoverable missing info from shared_ptr
    m_oicResourceInit->m_discoverable = sharePtr->isObservable();
    m_oicResourceInit->m_observable = sharePtr->isObservable();
    m_oicResourceInit->m_isSecure = false;
    m_sid =  sharePtr->sid();
    m_host =  sharePtr->host();
    m_idfull = m_host + sharePtr->uri();

    for (auto &resourceTypes : sharePtr->getResourceTypes()) {
        m_oicResourceInit->m_resourceTypeNameArray.push_back(
          resourceTypes.c_str());
    }

    for (auto &resourceInterfaces : sharePtr->getResourceInterfaces()) {
        m_oicResourceInit->m_resourceInterfaceArray.push_back(
          resourceInterfaces.c_str());
    }
}

int IotivityResourceClient::getResourceHandleToInt() {
    return static_cast<int>(m_id);
}

std::string IotivityResourceClient::getResourceId() {
    return m_idfull;
}


void IotivityResourceClient::serialize(picojson::object& object) {
    object["id"] = picojson::value(getResourceId());

    picojson::object properties;
    m_oicResourceInit->serialize(properties);
    object["OicResourceInit"] = picojson::value(properties);
}

void IotivityResourceClient::onPut(const HeaderOptions& headerOptions,
  const OCRepresentation& rep,
  const int eCode,
  double asyncCallId) {
    DEBUG_MSG("onPut: eCode=%d, asyncCallId=%f\n", eCode, asyncCallId);

    picojson::value::object object;
    object["cmd"] = picojson::value("updateResourceCompleted");
    object["eCode"] = picojson::value(
      static_cast<double>(eCode));
    object["asyncCallId"] = picojson::value(
      static_cast<double>(asyncCallId));

    if (eCode == SUCCESS_RESPONSE) {
        PrintfOcRepresentation(rep);
        m_oicResourceInit->m_resourceRep = rep;
        serialize(object);
    } else {
        ERROR_MSG("onPut was unsuccessful\n");
    }

    picojson::value value(object);
    m_device->PostMessage(value.serialize().c_str());
}

void IotivityResourceClient::onGet(const HeaderOptions& headerOptions,
  const OCRepresentation& rep,
  const int eCode, double asyncCallId) {
    DEBUG_MSG("onGet: eCode=%d, %f\n", eCode, asyncCallId);

    picojson::value::object object;
    object["cmd"] = picojson::value("retrieveResourceCompleted");
    object["eCode"] = picojson::value(
      static_cast<double>(eCode));

    object["asyncCallId"] = picojson::value(
      static_cast<double>(asyncCallId));

    if (eCode == SUCCESS_RESPONSE) {
        PrintfOcRepresentation(rep);
        m_oicResourceInit->m_resourceRep = rep;
        serialize(object);
    } else {
        ERROR_MSG("onGet was unsuccessful\n");
    }

    picojson::value value(object);
    m_device->PostMessage(value.serialize().c_str());
}

void IotivityResourceClient::onPost(
  const HeaderOptions& headerOptions,
  const OCRepresentation& rep,
  const int eCode, double asyncCallId) {
    DEBUG_MSG("onPost: eCode=%d, %f\n", eCode, asyncCallId);

    picojson::value::object object;
    object["cmd"] = picojson::value("createResourceCompleted");
    object["eCode"] = picojson::value(
      static_cast<double>(eCode));
    object["asyncCallId"] = picojson::value(
      static_cast<double>(asyncCallId));

    if (eCode == SUCCESS_RESPONSE) {
        PrintfOcRepresentation(rep);
        m_oicResourceInit->m_resourceRep = rep;
        serialize(object);
    } else {
        ERROR_MSG("onPost was unsuccessful\n");
    }

    picojson::value value(object);
    m_device->PostMessage(value.serialize().c_str());
}

void IotivityResourceClient::onObserve(
  const HeaderOptions headerOptions,
  const OCRepresentation& rep,
  const int& eCode,
  const int& sequenceNumber,
  double asyncCallId) {
    DEBUG_MSG("\n\n[Remote Server==>] "
              "onObserve: sequenceNumber=%d, eCode=%d, %f\n",
      sequenceNumber, eCode, asyncCallId);
    picojson::value::object object;
    object["cmd"] = picojson::value("onObserve");
    object["eCode"] = picojson::value(static_cast<double>(eCode));
    serialize(object);
    object["type"] = picojson::value("update");

    if (eCode == OC_STACK_OK) {
        PrintfOcRepresentation(rep);
        m_oicResourceInit->m_resourceRep = rep;
        serialize(object);

        std::vector<std::string> updatedPropertyNames;
        for (auto& cur : rep) {
            std::string attrname = cur.attrname();
            updatedPropertyNames.push_back(attrname);
        }

        picojson::array updatedPropertyNamesArray;
        CopyInto(updatedPropertyNames, updatedPropertyNamesArray);
        object["updatedPropertyNames"] =
         picojson::value(updatedPropertyNamesArray);
    } else {
        ERROR_MSG("\n\n[Remote Server==>] onObserve: error\n");
    }

    picojson::value value(object);
    m_device->PostMessage(value.serialize().c_str());
}

void IotivityResourceClient::onDelete(
  const HeaderOptions& headerOptions, const int eCode,
  double asyncCallId) {
    DEBUG_MSG("onDelete: eCode=%d, %f\n", eCode, asyncCallId);

    picojson::value::object object;
    object["cmd"] = picojson::value("deleteResourceCompleted");
    object["eCode"] = picojson::value(static_cast<double>(eCode));
    object["asyncCallId"] = picojson::value(
      static_cast<double>(asyncCallId));

    if (eCode == SUCCESS_RESPONSE) {
      // TODO(aphao) handle success/log etc...
    } else {
        ERROR_MSG("onDelete was unsuccessful\n");
    }

    picojson::value value(object);
    m_device->PostMessage(value.serialize().c_str());
}

OCStackResult IotivityResourceClient::createResource(
  IotivityResourceInit & oicResourceInit, double asyncCallId) {
    DEBUG_MSG("createResource %f\n", asyncCallId);

    OCStackResult result = OC_STACK_ERROR;

    if (m_ocResourcePtr == NULL)
        return result;


    PostCallback attributeHandler = std::bind(&IotivityResourceClient::onPost,
                                    this,
                                    std::placeholders::_1,
                                    std::placeholders::_2,
                                    std::placeholders::_3,
                                    asyncCallId);
    result = m_ocResourcePtr->post(oicResourceInit.m_resourceRep,
                                   QueryParamsMap(),
                                   attributeHandler);
    if (OC_STACK_OK != result) {
        ERROR_MSG("post/create was unsuccessful\n");
        return result;
    }

    return result;
}

OCStackResult IotivityResourceClient::retrieveResource(double asyncCallId) {
    DEBUG_MSG("retrieveResource %f\n", asyncCallId);

    OCStackResult result = OC_STACK_ERROR;

    if (m_ocResourcePtr == NULL)
        return result;

    GetCallback attributeHandler = std::bind(&IotivityResourceClient::onGet,
                                   this,
                                   std::placeholders::_1,
                                   std::placeholders::_2,
                                   std::placeholders::_3,
                                   asyncCallId);
    result = m_ocResourcePtr->get(QueryParamsMap(),
                                  attributeHandler);
    if (OC_STACK_OK != result) {
        ERROR_MSG("get was unsuccessful\n");
        return result;
    }

    return result;
}

OCStackResult IotivityResourceClient::updateResource(
  OCRepresentation & representation, double asyncCallId) {
    DEBUG_MSG("updateResource %f\n", asyncCallId);

    OCStackResult result = OC_STACK_ERROR;

    if (m_ocResourcePtr == NULL)
        return result;

    PrintfOcRepresentation(representation);
    PutCallback attributeHandler = std::bind(&IotivityResourceClient::onPut,
                                   this,
                                   std::placeholders::_1,
                                   std::placeholders::_2,
                                   std::placeholders::_3,
                                   asyncCallId);
    result = m_ocResourcePtr->put(representation,
                                  QueryParamsMap(),
                                  attributeHandler);
    if (OC_STACK_OK != result) {
        ERROR_MSG("update was unsuccessful\n");
        return result;
    }

    return result;
}

OCStackResult IotivityResourceClient::deleteResource(double asyncCallId) {
    DEBUG_MSG("deleteResource %f\n", asyncCallId);

    OCStackResult result = OC_STACK_ERROR;

    if (m_ocResourcePtr == NULL)
        return result;

    DeleteCallback deleteHandler = std::bind(&IotivityResourceClient::onDelete,
                                   this,
                                   std::placeholders::_1,
                                   std::placeholders::_2,
                                   asyncCallId);
    result = m_ocResourcePtr->deleteResource(deleteHandler);

    if (OC_STACK_OK != result) {
        ERROR_MSG("delete was unsuccessful\n");
        return result;
    }

    return result;
}

OCStackResult IotivityResourceClient::startObserving(double asyncCallId) {
    DEBUG_MSG("startObserving %f\n", asyncCallId);

    OCStackResult result = OC_STACK_ERROR;

    if (m_ocResourcePtr == NULL)
        return result;

    ObserveCallback observeHandler =
        std::bind(&IotivityResourceClient::onObserve,
                  this,
                  std::placeholders::_1,
                  std::placeholders::_2,
                  std::placeholders::_3,
                  std::placeholders::_4,
                  asyncCallId);

    result = m_ocResourcePtr->observe(ObserveType::ObserveAll,
                                      QueryParamsMap(),
                                      observeHandler);
    if (OC_STACK_OK != result) {
        ERROR_MSG("observe was unsuccessful\n");
        return result;
    }

    return result;
}

OCStackResult IotivityResourceClient::cancelObserving(double asyncCallId) {
    DEBUG_MSG("cancelObserving %f\n", asyncCallId);
    OCStackResult result = OC_STACK_ERROR;

    if (m_ocResourcePtr == NULL)
        return result;

    result = m_ocResourcePtr->cancelObserve();
    if (OC_STACK_OK != result) {
        ERROR_MSG("cancelObserve was unsuccessful\n");
        return result;
    }

    return result;
}

IotivityRequestEvent::IotivityRequestEvent() {}

IotivityRequestEvent::~IotivityRequestEvent() {}

void IotivityRequestEvent::deserialize(
  std::shared_ptr<OCResourceRequest> request) {
    std::string requestType = request->getRequestType();
    int requestFlag = request->getRequestHandlerFlag();
    int requestHandle = (int)request->getRequestHandle(); // NOLINT

    DEBUG_MSG("Deserialize: requestType=%s\n", requestType.c_str());
    DEBUG_MSG("Deserialize: requestFlag=0x%x\n", requestFlag);
    DEBUG_MSG("Deserialize: requestHandle=0x%x\n", requestHandle);

    if (requestFlag & RequestHandlerFlag::RequestFlag) {
        if (requestType == "GET") {
            m_type = "retrieve";
        } else if (requestType == "PUT") {
            m_type = "update";
        } else if (requestType == "POST") {
            // m_type = "create";
            // TODO(aphao) : or update
            m_type = "update";
        } else if (requestType == "DELETE") {
            m_type = "delete";
        }

        m_resourceRep = request->getResourceRepresentation();
        PrintfOcRepresentation(m_resourceRep);

        for (auto& cur : m_resourceRep) {
            std::string attrname = cur.attrname();
            m_updatedPropertyNames.push_back(attrname);
        }
    }

    m_queries = request->getQueryParameters();

    if (!m_queries.empty()) {
        for (auto it : m_queries) {
            DEBUG_MSG("Queries: key=%s, value=%s\n",
              it.first.c_str(), it.second.c_str());
        }
    }

    m_headerOptions = request->getHeaderOptions();

    if (!m_headerOptions.empty()) {
        for (auto it = m_headerOptions.begin();
             it != m_headerOptions.end(); ++it) {
            DEBUG_MSG("HeaderOptions: ID=%d, value=%s\n",
              it->getOptionID(),
              it->getOptionData().c_str());
        }
    }

    m_requestId = requestHandle;
    m_source = std::to_string(requestHandle);
    m_target = std::to_string((int)(request->getResourceHandle())); // NOLINT
}

void IotivityRequestEvent::deserialize(const picojson::value& value) {
    m_requestId = static_cast<int>(value.get("requestId").get<double>());
    m_type = value.get("type").to_str();
    m_source = value.get("source").to_str();
    m_target = value.get("target").to_str();
    picojson::value properties = value.get("properties");
    picojson::object & propertiesobject = properties.get<picojson::object>();

    DEBUG_MSG("properties: size=%d\n", propertiesobject.size());

    for (picojson::value::object::iterator iter = propertiesobject.begin();
            iter != propertiesobject.end();
            ++iter) {
        std::string objectKey = iter->first;
        picojson::value objectValue = iter->second;

        if (objectValue.is<bool>()) {
            DEBUG_MSG("[bool] key=%s, value=%d\n",
              objectKey.c_str(),
              objectValue.get<bool>());
            m_resourceRep[objectKey] = objectValue.get<bool>();
        } else if (objectValue.is<double>()) {
            DEBUG_MSG("[double] key=%s, value=%f\n",
              objectKey.c_str(),
              objectValue.get<double>());
            m_resourceRep[objectKey] = objectValue.get<double>();
        } else if (objectValue.is<string>()) {
            DEBUG_MSG("[string] key=%s, value=%s\n",
              objectKey.c_str(),
              objectValue.get<string>().c_str());
            m_resourceRep[objectKey] = objectValue.get<string>();
        } else if (objectValue.is<int>()) {
            DEBUG_MSG("[string] key=%s, value=%d\n",
              objectKey.c_str(),
              objectValue.get<int>());
            m_resourceRep[objectKey] = objectValue.get<int>();
       }
    }
}

void IotivityRequestEvent::serialize(picojson::object& object) {
    object["type"] = picojson::value(m_type);
    object["requestId"] = picojson::value(static_cast<double>(m_requestId));
    object["source"] = picojson::value(m_source);
    object["target"] = picojson::value(m_target);

    if ((m_type == "create") || (m_type == "update")) {
        picojson::object properties;
        TranslateOCRepresentationToPicojson(m_resourceRep, properties);
        object["properties"] = picojson::value(properties);

        PrintfOcRepresentation(m_resourceRep);
    }

    if ((m_type == "retrieve") || (m_type == "observe")) {
        picojson::object properties;
        TranslateOCRepresentationToPicojson(m_resourceRepTarget, properties);
        object["properties"] = picojson::value(properties);

        PrintfOcRepresentation(m_resourceRepTarget);
    }

    if (m_type == "update") {
        picojson::array updatedPropertyNamesArray;

        CopyInto(m_updatedPropertyNames, updatedPropertyNamesArray);
        object["updatedPropertyNames"] =
          picojson::value(updatedPropertyNamesArray);
    }

    picojson::array queryOptionsArray;
    if (!m_queries.empty()) {
        for (auto it : m_queries) {
            picojson::object object;
            DEBUG_MSG("Queries: key=%s, value=%s\n",
              it.first.c_str(),
              it.second.c_str());
            object[it.first.c_str()] = picojson::value(it.second);
            queryOptionsArray.push_back(picojson::value(object));
        }
    }

    picojson::array headerOptionsArray;

    if (!m_headerOptions.empty()) {
        picojson::object object;

        for (auto it = m_headerOptions.begin();
             it != m_headerOptions.end(); ++it) {
            picojson::object object;
            DEBUG_MSG("HeaderOptions: ID=%d, value=%s\n",
              it->getOptionID(),
              it->getOptionData().c_str());
            object[std::to_string(it->getOptionID()).c_str()] =
              picojson::value(it->getOptionData());
            headerOptionsArray.push_back(picojson::value(object));
        }
    }
}

OCStackResult IotivityRequestEvent::sendResponse() {
    DEBUG_MSG("handleSendResponse: type=%s\n", m_type.c_str());

    OCStackResult result = OC_STACK_ERROR;
    auto pResponse = std::make_shared<OC::OCResourceResponse>();
    pResponse->setRequestHandle(reinterpret_cast<void *>(m_requestId));

    int targetId = atoi(m_target.c_str());
    pResponse->setResourceHandle(reinterpret_cast<void *>(targetId));

    if (m_type == "retrieve") {
        // Send targetId representation
        pResponse->setResourceRepresentation(m_resourceRep);
        pResponse->setErrorCode(200);
        pResponse->setResponseResult(OC_EH_OK);
    } else if (m_type == "update") {
        pResponse->setResourceRepresentation(m_resourceRep);
        pResponse->setErrorCode(200);
        pResponse->setResponseResult(OC_EH_OK);
    } else if (m_type == "create") {
        // TODO(aphao) POST (with special flags ??)
        // OCRepresentation rep = pRequest->getResourceRepresentation();
        // OCRepresentation rep_post = post(rep);
        pResponse->setResourceRepresentation(m_resourceRep);
        pResponse->setErrorCode(200);
        pResponse->setResponseResult(OC_EH_OK);
    } else if (m_type == "observe") {
        pResponse->setResourceRepresentation(m_resourceRep);
        pResponse->setErrorCode(200);
        pResponse->setResponseResult(OC_EH_OK);
    }

    pResponse->setHeaderOptions(m_headerOptions);
    result = OCPlatform::sendResponse(pResponse);

    if (OC_STACK_OK != result) {
        ERROR_MSG("OCPlatform::sendResponse was unsuccessful\n");
    }

    return result;
}

OCStackResult IotivityRequestEvent::sendError() {
    OCStackResult result = OC_STACK_ERROR;
    auto pResponse = std::make_shared<OC::OCResourceResponse>();
    pResponse->setRequestHandle(reinterpret_cast<void *>(m_requestId));

    int targetId = atoi(m_target.c_str());
    pResponse->setResourceHandle(reinterpret_cast<void *>(targetId));

    // value.get("error").to_str()
    pResponse->setErrorCode(200);
    pResponse->setResponseResult(OC_EH_ERROR);
    result = OCPlatform::sendResponse(pResponse);

    if (OC_STACK_OK != result) {
        ERROR_MSG("OCPlatform::sendError was unsuccessful\n");
    }

    return result;
}

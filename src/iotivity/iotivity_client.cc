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
#include "iotivity/iotivity_client.h"
#include "iotivity/iotivity_device.h"

#include "iotivity/iotivity_resource.h"


IotivityClient::IotivityClient(IotivityDevice* device) : m_device(device) {

  m_asyncCallId_findresources = 0;
  m_asyncCallId_finddevice = 0;
}

IotivityClient::~IotivityClient() {

}

void *IotivityClient::getResourceById(int id) {

  if (m_resourcemap.size()) {

    std::map<int, void *>::const_iterator it;
    if ((it = m_resourcemap.find(id)) != m_resourcemap.end())
      return (void *)((*it).second);
  }

  return NULL;
}

void IotivityClient::foundResourceCallback(std::shared_ptr<OCResource> resource) {

    DEBUG_MSG("foundResourceCallback:\n");

    IotivityResourceClient *oicResourceClient = new IotivityResourceClient(m_device);
    oicResourceClient->setSharedPtr(resource);

    m_resourcemap[oicResourceClient->getResourceHandleToInt()] = (void *)oicResourceClient;

    picojson::value::object object;
    object["cmd"] = picojson::value("foundResourceCallback");
    object["asyncCallId"] = picojson::value(m_asyncCallId_findresources);
    m_asyncCallId_findresources = 0;

    PrintfOcResource((const OCResource &)*resource);

    oicResourceClient->serialize(object);
 
    picojson::value value(object);
    m_device->PostMessage(value.serialize().c_str());
}

void IotivityClient::handleFindResources(const picojson::value& value) {

    DEBUG_MSG("handleFindResources: v=%s\n",value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    picojson::value param = value.get("OicDiscoveryOptions");

    m_asyncCallId_findresources = async_call_id;

    // all properties are null by default, meaning “find all”
    // if resourceId is specified in full form, a direct retrieve is made
    // if resourceType is specified, a retrieve on /oic/res is made
    // if resourceId is null, and deviceId not, then only resources from that device are returned
    std::string deviceId = param.get("deviceId").to_str();
    std::string resourceId = param.get("resourceId").to_str();
    std::string resourceType = param.get("resourceType").to_str();
 
    // TODO: Enhanced to global super timer multi server search !!!
    // TODO: find resources according to DiscoveryOptions {deviceId, resourceId, resourceType}

    std:string hostUri = OC_MULTICAST_DISCOVERY_URI;
    string requestUri = hostUri + "?rt=" + resourceType;

    FindCallback resourceHandler = std::bind(&IotivityClient::foundResourceCallback, 
                                             this, 
                                             std::placeholders::_1);
    OCStackResult result = OCPlatform::findResource("", 
                                                    requestUri,
                                                    OC_ALL, 
                                                    resourceHandler);
    if (OC_STACK_OK != result)
    {
        std::cerr << "OCPlatform::findResource was unsuccessful\n";
        m_device->postError(async_call_id);
        return;
    }
}

void IotivityClient::foundDeviceCallback(const OCRepresentation& rep) {

    DEBUG_MSG("foundDeviceCallback:\n");

    // TODO debug
    picojson::value::object object;
    object["cmd"] = picojson::value("foundDeviceCallback");
    object["asyncCallId"] = picojson::value(m_asyncCallId_finddevice);
    m_asyncCallId_finddevice = 0;

  
    picojson::value value(object);
    m_device->PostMessage(value.serialize().c_str());
}

void IotivityClient::handleFindDevices(const picojson::value& value) {

    DEBUG_MSG("handleFindDevices: v=%s\n",value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    picojson::value param = value.get("OicDiscoveryOptions");

    m_asyncCallId_finddevice = async_call_id;

    std::string deviceId = param.get("deviceId").to_str();
    std::string resourceId = param.get("resourceId").to_str();
    std::string resourceType = param.get("resourceType").to_str();

    OCConnectivityType connectivityType = OC_IPV4;
    FindDeviceCallback deviceInfoHandler = std::bind(&IotivityClient::foundDeviceCallback, 
                                                    this, 
                                                    std::placeholders::_1);
    OCStackResult result = OCPlatform::getDeviceInfo(deviceId, 
                                                     resourceId,
                                                     connectivityType, 
                                                     deviceInfoHandler);
    if (OC_STACK_OK != result)
    {
        std::cerr << "OCPlatform::getDeviceInfo was unsuccessful\n";
        m_device->postError(async_call_id);
        return;
    }
}

void IotivityClient::handleCreateResource(const picojson::value& value) {

    // Post + particular data
    DEBUG_MSG("handleCreateResource: v=%s\n",value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();

    IotivityResourceInit oicResourceInit(value.get("OicResourceInit"));

    int resHandleInt = (int)atoi(oicResourceInit.m_deviceId.c_str());

    IotivityResourceClient *oicResourceClient = (IotivityResourceClient *)getResourceById(resHandleInt);
    if (oicResourceClient != NULL)
    {
        oicResourceClient->m_asyncCallId_create = async_call_id;
        OCStackResult result = oicResourceClient->createResource(oicResourceInit);
        if (OC_STACK_OK != result)
        {
            m_device->postError(async_call_id);
            return;
        }
    }
    else
    {
        m_device->postError(async_call_id);
    }
}

void IotivityClient::handleRetrieveResource(const picojson::value& value) {
    DEBUG_MSG("handleRetrieveResource: v=%s\n",value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    int resHandleInt = (int)value.get("resourceId").get<double>();

    IotivityResourceClient *oicResourceClient = (IotivityResourceClient *)getResourceById(resHandleInt);
    if (oicResourceClient != NULL)
    {
        oicResourceClient->m_asyncCallId_retrieve = async_call_id;
        OCStackResult result = oicResourceClient->retrieveResource();
        if (OC_STACK_OK != result)
        {
            m_device->postError(async_call_id);
            return;
        }
    }
    else
    {
        m_device->postError(async_call_id);
    }
}

void IotivityClient::handleUpdateResource(const picojson::value& value) {

    DEBUG_MSG("handleUpdateResource: v=%s\n",value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    picojson::value param = value.get("OicResource");
    std::string deviceId = param.get("deviceId").to_str();
    int resHandleInt = (int)atoi(deviceId.c_str());

    IotivityResourceClient *oicResourceClient = (IotivityResourceClient *)getResourceById(resHandleInt);
    if (oicResourceClient != NULL)
    {
        oicResourceClient->m_asyncCallId_update = async_call_id;
        IotivityResourceInit oicResourceInit(param);

        OCStackResult result = oicResourceClient->updateResource(oicResourceInit.m_resourceRep);
        if (OC_STACK_OK != result)
        {
            m_device->postError(async_call_id);
            return;
        }
    }
    else
    {
        m_device->postError(async_call_id);
    }
}

void IotivityClient::handleDeleteResource(const picojson::value& value) {
    DEBUG_MSG("handleDeleteResource: v=%s\n",value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    int resHandleInt = (int)value.get("resourceId").get<double>();
    IotivityResourceClient *oicResourceClient = (IotivityResourceClient *)getResourceById(resHandleInt);
    if (oicResourceClient != NULL)
    {
        oicResourceClient->m_asyncCallId_delete = async_call_id;
        OCStackResult result = oicResourceClient->deleteResource();
        if (OC_STACK_OK != result)
        {
            m_device->postError(async_call_id);
            return;
        }
    }
    else
    {
        m_device->postError(async_call_id);
    }
}

void IotivityClient::handleStartObserving(const picojson::value& value) {

    DEBUG_MSG("handleStartObserving: v=%s\n",value.serialize().c_str());
    OCStackResult result;

    double async_call_id = value.get("asyncCallId").get<double>();
    int resHandleInt = (int)value.get("resourceId").get<double>();

    IotivityResourceClient *oicResourceClient = (IotivityResourceClient *)getResourceById(resHandleInt);
    if (oicResourceClient != NULL)
    {
        oicResourceClient->m_asyncCallId_observe = async_call_id;
        result = oicResourceClient->startObserving();
        if (OC_STACK_OK != result)
        {
            m_device->postError(async_call_id);
            return;
        }
    }
    else
    {
        m_device->postError(async_call_id);
        return;
    }

    // retrieve + observe flag
    result = oicResourceClient->retrieveResource();
}

void IotivityClient::handleCancelObserving(const picojson::value& value) {

    DEBUG_MSG("handleCancelObserving: v=%s\n",value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    int resHandleInt = (int)value.get("resourceId").get<double>();

    IotivityResourceClient *oicResourceClient = (IotivityResourceClient *)getResourceById(resHandleInt);
    if (oicResourceClient != NULL)
    {
        OCStackResult result = oicResourceClient->cancelObserving();
        if (OC_STACK_OK != result)
        {
            m_device->postError(async_call_id);
            return;
        }
    }
    else
    {
        m_device->postError(async_call_id);
        return;
    }
 
    m_device->postResult("cancelObservingCompleted", async_call_id);
}


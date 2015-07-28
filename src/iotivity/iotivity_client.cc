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
#include <string>
#include <map>

#include "iotivity/iotivity_client.h"
#include "iotivity/iotivity_device.h"
#include "iotivity/iotivity_resource.h"


IotivityClient::IotivityClient(IotivityDevice* device) : m_device(device) {
    m_asyncCallId_findresources = 0;
    m_asyncCallId_finddevices = 0;
    m_discoveryOptionDeviceId = "";
}

IotivityClient::~IotivityClient() {
    for (auto const &entity : m_resourcemap) {
        IotivityResourceClient *resClient =
            reinterpret_cast<IotivityResourceClient *>(entity.second);
        delete resClient;
    }
    m_resourcemap.clear();
    m_foundresourcemap.clear();
}

void *IotivityClient::getResourceById(int id) {
    if (m_resourcemap.size()) {
        std::map<int, void *>::const_iterator it;
        if ((it = m_resourcemap.find(id)) != m_resourcemap.end())
            return reinterpret_cast<void *>((*it).second);
    }

    return NULL;
}

void IotivityClient::foundResourceCallback(std::shared_ptr<OCResource> resource,
        int waitsec) {
    DEBUG_MSG("foundResourceCallback:\n");
    bool mapFoundResource = true;

    IotivityResourceClient *resClient = new IotivityResourceClient(m_device);
    resClient->setSharedPtr(resource);
    PrintfOcResource((const OCResource &)*resource);

    if (m_discoveryOptionDeviceId != "") {
        if (resource->sid() != m_discoveryOptionDeviceId) {
            mapFoundResource = false;
        }
    }

    if (mapFoundResource) {
    m_foundresourcemap[resClient->getResourceHandleToInt()] =
        reinterpret_cast<void *>(resClient);
    }

    if (waitsec == -1) {
        findPreparedRequest();
    }
}

void IotivityClient::handleFindResources(const picojson::value& value) {
    DEBUG_MSG("handleFindResources: v=%s\n", value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    picojson::value param = value.get("OicDiscoveryOptions");
    m_asyncCallId_findresources = async_call_id;

    // all properties are null by default, meaning “find all”
    // if resourceId is specified in full form, a direct retrieve is made
    // if resourceType is specified, a retrieve on /oic/res is made
    // if resourceId is null, and deviceId not,
    // then only resources from that device are returned
    std::string deviceId = "";
    std::string resourceId = "";
    std::string resourceType = "";
    int waitsec = 5;

    if (param.contains("deviceId")) {
        deviceId = param.get("deviceId").to_str();
    }

    if (param.contains("resourceId")) {
        resourceId = param.get("resourceId").to_str();
    }

    if (param.contains("resourceType")) {
        resourceType = param.get("resourceType").to_str();
    }

    if (param.contains("waitsec")) {
        waitsec = static_cast<int>(param.get("waitsec").get<double>());
    }

    // TODO(aphao) centralize all debug msg
    DEBUG_MSG("handleFindResources: device = %s\n", deviceId.c_str());
    DEBUG_MSG("\tresource = %s\n", resourceId.c_str());
    DEBUG_MSG("\tresourceType = %s\n", resourceType.c_str());
    DEBUG_MSG("\ttimeout = %d\n", waitsec);

    m_foundresourcemap.clear();
    m_discoveryOptionDeviceId = "";

    std::string hostUri = "";
    std: string discoveryUri = OC_MULTICAST_DISCOVERY_URI;  //  /oc/core
    string requestUri = discoveryUri;

    if ((deviceId == "") && (resourceId == "") && (resourceType == "")) {
        // Find all
    } else {
        if (resourceType != "") {
            requestUri = discoveryUri + "?rt=" + resourceType;
        }

        if (resourceId != "") {
            int resId = atoi(resourceId.c_str());
            void * rId = getResourceById(resId);
            IotivityResourceClient *resClient =
                reinterpret_cast<IotivityResourceClient *>(rId);
            if (resClient == NULL) {
                ERROR_MSG(
                "OCPlatform::findResource by resourceId was unsuccessful\n");
                m_device->postError(async_call_id);
                return;
            } else {
                m_foundresourcemap[resId] =
                reinterpret_cast<void *>(resClient);
                findPreparedRequest();
                return;
            }
        } else {
            if (deviceId != "") {
                m_discoveryOptionDeviceId = deviceId;
            }
        }
    }

    DEBUG_MSG("process: hostUri=%s, resId=%s, resType=%s, "
              "deviceId=%s, timeout=%ds\n",
              hostUri.c_str(),
              resourceId.c_str(),
              resourceType.c_str(),
              deviceId.c_str(),
              waitsec);

    FindCallback resourceHandler =
        std::bind(&IotivityClient::foundResourceCallback,
                  this,
                  std::placeholders::_1,
                  waitsec);
    OCStackResult result = OCPlatform::findResource(hostUri,
                           requestUri,
                           OC_ALL,
                           resourceHandler);
    if (OC_STACK_OK != result) {
        ERROR_MSG("OCPlatform::findResource was unsuccessful\n");
        m_device->postError(async_call_id);
        return;
    }

    if (waitsec >= 0) {
        std::thread exec(
            std::function< void(int second) >(
                std::bind(&IotivityClient::findResourceTimerCallback,
                          this,
                          std::placeholders::_1)),
            waitsec);
        exec.detach();
    }
}

void IotivityClient::findResourceTimerCallback(int waitsec) {
    sleep(waitsec);
    findPreparedRequest();
}

void IotivityClient::findPreparedRequest() {
    std::lock_guard<std::mutex> lock(m_callbackLock);
    picojson::value::object object;
    object["cmd"] = picojson::value("foundResourceCallback");
    object["asyncCallId"] = picojson::value(m_asyncCallId_findresources);
    m_asyncCallId_findresources = 0;
    picojson::array resourcesArray;

    for (auto const &entity : m_foundresourcemap) {
        IotivityResourceClient *resClient =
            reinterpret_cast<IotivityResourceClient *>(entity.second);
        picojson::object resourceobj;
        resClient->serialize(resourceobj);
        resourcesArray.push_back(picojson::value(resourceobj));
        m_resourcemap[entity.first] = entity.second;
    }

    object["resourcesArray"] = picojson::value(resourcesArray);
    picojson::value value(object);
    m_device->PostMessage(value.serialize().c_str());
}

void IotivityClient::foundDeviceCallback(const OCRepresentation& rep,
                                         int waitsec) {
    DEBUG_MSG("foundDeviceCallback:\n");

    // TODO(aphao) triggered but nothing in OCRepresentation
    // Add device in
    PrintfOcRepresentation(rep);

    if (waitsec == -1) {
        findDevicePreparedRequest();
    }
}

void IotivityClient::handleFindDevices(const picojson::value& value) {
    DEBUG_MSG("handleFindDevices: v=%s\n", value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    picojson::value param = value.get("OicDiscoveryOptions");
    m_asyncCallId_finddevices = async_call_id;


    // all properties are null by default, meaning “find all”
    // if resourceId is specified in full form, a direct retrieve is made
    // if resourceType is specified, a retrieve on /oic/res is made
    // if resourceId is null, and deviceId not,
    // then only resources from that device are returned
    std::string deviceId = "";
    std::string resourceId = "";
    std::string resourceType = "";
    int waitsec = 5;

    if (param.contains("deviceId")) {
        deviceId = param.get("deviceId").to_str();
    }

    if (param.contains("resourceId")) {
        resourceId = param.get("resourceId").to_str();
    }

    if (param.contains("resourceType")) {
        resourceType = param.get("resourceType").to_str();
    }

    if (param.contains("waitsec")) {
        waitsec = static_cast<int>(param.get("waitsec").get<double>());
    }

    // TODO(aphao) centralize all debug msg
    DEBUG_MSG("handleFindDevices: device = %s\n", deviceId.c_str());
    DEBUG_MSG("\tresource = %s\n", resourceId.c_str());
    DEBUG_MSG("\tresourceType = %s\n", resourceType.c_str());
    DEBUG_MSG("\ttimeout = %d\n", waitsec);

    m_founddevicemap.clear();

    OCConnectivityType connectivityType = OC_IPV4;

    std::string hostUri = "";  //  multicast
    std::string deviceDiscoveryURI   = "/oci/d";
    std::string deviceDiscoveryRequest = OC_MULTICAST_PREFIX
                                       + deviceDiscoveryURI;

    DEBUG_MSG("process: hostUri=%s, request=%s, timeout=%ds\n",
              hostUri.c_str(),
              deviceDiscoveryRequest.c_str(),
              waitsec);
    FindDeviceCallback deviceInfoHandler =
        std::bind(&IotivityClient::foundDeviceCallback,
                  this,
                  std::placeholders::_1,
                  waitsec);
    OCStackResult result = OCPlatform::getDeviceInfo(hostUri,
                           deviceDiscoveryRequest.c_str(),
                           connectivityType,
                           deviceInfoHandler);
    if (OC_STACK_OK != result) {
        ERROR_MSG("OCPlatform::getDeviceInfo was unsuccessful\n");
        m_device->postError(async_call_id);
        return;
    }

    if (waitsec >= 0) {
        std::thread exec(
            std::function< void(int second) >(
                std::bind(&IotivityClient::findDeviceTimerCallback,
                          this,
                          std::placeholders::_1)),
            waitsec);
        exec.detach();
    }
}

void IotivityClient::findDeviceTimerCallback(int waitsec) {
    sleep(waitsec);
    findDevicePreparedRequest();
}

void IotivityClient::findDevicePreparedRequest() {
    std::lock_guard<std::mutex> lock(m_callbackLockDevices);
    picojson::value::object object;
    object["cmd"] = picojson::value("foundDeviceCallback");
    object["asyncCallId"] = picojson::value(m_asyncCallId_finddevices);
    m_asyncCallId_finddevices = 0;
    picojson::array devicesArray;

    for (auto const &entity : m_founddevicemap) {
        IotivityResourceClient *resClient =
            reinterpret_cast<IotivityResourceClient *>(entity.second);
        picojson::object deviceobj;
        resClient->serialize(deviceobj);
        devicesArray.push_back(picojson::value(deviceobj));
        m_devicemap[entity.first] = entity.second;
    }

    object["devicesArray"] = picojson::value(devicesArray);
    picojson::value value(object);
    m_device->PostMessage(value.serialize().c_str());
}

void IotivityClient::handleCreateResource(const picojson::value& value) {
    // Post + particular data
    DEBUG_MSG("handleCreateResource: v=%s\n", value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    IotivityResourceInit oicResourceInit(value.get("OicResourceInit"));
    int resId = static_cast<int>(value.get("resourceId").get<double>());
    IotivityResourceClient *resClient =
        reinterpret_cast<IotivityResourceClient *>(getResourceById(resId));

    if (resClient != NULL) {
        resClient->m_asyncCallId_create = async_call_id;
        OCStackResult result = resClient->createResource(oicResourceInit);
        if (OC_STACK_OK != result) {
            m_device->postError(async_call_id);
            return;
        }
    } else {
        m_device->postError(async_call_id);
    }
}

void IotivityClient::handleRetrieveResource(const picojson::value& value) {
    DEBUG_MSG("handleRetrieveResource: v=%s\n", value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    int resId = static_cast<int>(value.get("resourceId").get<double>());
    IotivityResourceClient *resClient =
        reinterpret_cast<IotivityResourceClient *>(getResourceById(resId));

    if (resClient != NULL) {
        resClient->m_asyncCallId_retrieve = async_call_id;
        OCStackResult result = resClient->retrieveResource();
        if (OC_STACK_OK != result) {
            m_device->postError(async_call_id);
            return;
        }
    } else {
        m_device->postError(async_call_id);
    }
}

void IotivityClient::handleUpdateResource(const picojson::value& value) {
    DEBUG_MSG("handleUpdateResource: v=%s\n", value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    picojson::value param = value.get("OicResource");
    std::string deviceId = param.get("id").to_str();
    int resId = static_cast<int>(atoi(deviceId.c_str()));
    IotivityResourceClient *resClient =
        reinterpret_cast<IotivityResourceClient *>(getResourceById(resId));

    if (resClient != NULL) {
        resClient->m_asyncCallId_update = async_call_id;
        IotivityResourceInit oicResourceInit(param);

        OCStackResult result =
        resClient->updateResource(oicResourceInit.m_resourceRep);

        if (OC_STACK_OK != result) {
            m_device->postError(async_call_id);
            return;
        }
    } else {
        m_device->postError(async_call_id);
    }
}

void IotivityClient::handleDeleteResource(const picojson::value& value) {
    DEBUG_MSG("handleDeleteResource: v=%s\n", value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    int resId = static_cast<int>(value.get("resourceId").get<double>());
    IotivityResourceClient *resClient =
        reinterpret_cast<IotivityResourceClient *>(getResourceById(resId));

    if (resClient != NULL) {
        resClient->m_asyncCallId_delete = async_call_id;
        OCStackResult result = resClient->deleteResource();
        if (OC_STACK_OK != result) {
            m_device->postError(async_call_id);
            return;
        }
    } else {
        m_device->postError(async_call_id);
    }
}

void IotivityClient::handleStartObserving(const picojson::value& value) {
    DEBUG_MSG("handleStartObserving: v=%s\n", value.serialize().c_str());

    OCStackResult result;
    double async_call_id = value.get("asyncCallId").get<double>();
    int resId = static_cast<int>(value.get("resourceId").get<double>());
    IotivityResourceClient *resClient =
    reinterpret_cast<IotivityResourceClient *>(getResourceById(resId));

    if (resClient != NULL) {
        resClient->m_asyncCallId_observe = async_call_id;
        result = resClient->startObserving();
        if (OC_STACK_OK != result) {
            m_device->postError(async_call_id);
            return;
        }
    } else {
        m_device->postError(async_call_id);
        return;
    }

    // retrieve + observe flag
    result = resClient->retrieveResource();
}

void IotivityClient::handleCancelObserving(const picojson::value& value) {
    DEBUG_MSG("handleCancelObserving: v=%s\n", value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    int resId = static_cast<int>(value.get("resourceId").get<double>());
    IotivityResourceClient *resClient =
        reinterpret_cast<IotivityResourceClient *>(getResourceById(resId));

    if (resClient != NULL) {
        OCStackResult result = resClient->cancelObserving();
        if (OC_STACK_OK != result) {
            m_device->postError(async_call_id);
            return;
        }
    } else {
        m_device->postError(async_call_id);
        return;
    }
    m_device->postResult("cancelObservingCompleted", async_call_id);
}


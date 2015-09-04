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
        IotivityResourceClient *resClient = entity.second;
        delete resClient;
    }
    m_resourcemap.clear();
    m_foundresourcemap.clear();
}

IotivityResourceClient *IotivityClient::getResourceById(std::string id) {
    if (m_resourcemap.size()) {
        std::map<std::string, IotivityResourceClient *>::const_iterator it;
        if ((it = m_resourcemap.find(id)) != m_resourcemap.end())
            return (*it).second;
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
        m_foundresourcemap[resClient->getResourceId()] = resClient;
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

    DEBUG_MSG("handleFindResources: device = %s\n"
              "\tresource = %s\n"
              "\tresourceType = %s\n"
              "\ttimeout = %d\n",
              deviceId.c_str(),resourceId.c_str(),
              resourceType.c_str(), waitsec);

    m_foundresourcemap.clear();
    m_discoveryOptionDeviceId = "";

    std::string hostUri = "";
    std: string discoveryUri = OC_RSRVD_WELL_KNOWN_URI;
    string requestUri = discoveryUri;

    if ((deviceId == "") && (resourceId == "") && (resourceType == "")) {
        // Find all
    } else {
        if (resourceType != "") {
            requestUri = discoveryUri + "?rt=" + resourceType;
        }

        if (resourceId != "") {
            void * rId = getResourceById(resourceId);
            IotivityResourceClient *resClient = getResourceById(resourceId);
            if (resClient == NULL) {
                ERROR_MSG(
                "OCPlatform::findResource by resourceId was unsuccessful\n");
                m_device->postError(async_call_id);
                return;
            } else {
                m_foundresourcemap[resourceId] = resClient;
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
                           CT_DEFAULT,
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
        IotivityResourceClient *resClient = entity.second;
        picojson::object resourceobj;
        resClient->serialize(resourceobj);
        resourcesArray.push_back(picojson::value(resourceobj));
        m_resourcemap[entity.first] = entity.second;
    }

    object["resourcesArray"] = picojson::value(resourcesArray);
    picojson::value value(object);
    m_device->PostMessage(value.serialize().c_str());
}

void IotivityClient::foundPlatformCallback(const OCRepresentation& rep) {
    DEBUG_MSG("foundPlatformCallback\n");

    std::string value;
    std::string values[] = {
        "pi",   "Platform ID                    ",
        "mnmn", "Manufacturer name              ",
        "mnml", "Manufacturer url               ",
        "mnmo", "Manufacturer Model No          ",
        "mndt", "Manufactured Date              ",
        "mnpv", "Manufacturer Platform Version  ",
        "mnos", "Manufacturer OS version        ",
        "mnhw", "Manufacturer hardware version  ",
        "mnfv", "Manufacturer firmware version  ",
        "mnsl", "Manufacturer support url       ",
        "st",   "Manufacturer system time       "
    };

    for (unsigned int i = 0; i < sizeof(values) / sizeof(values[0]) ; i += 2) {
        if (rep.getValue(values[i], value)) {
            DEBUG_MSG("\t%s:%s\n", values[i + 1].c_str(), value.c_str());
        }
    }

    //IotivityDeviceInfo *deviceInfo = new IotivityDeviceInfo();
    //m_founddevicemap[] = deviceInfo;

}

void IotivityClient::foundDeviceCallback(const OCRepresentation& rep,
                                         int waitsec) {
    DEBUG_MSG("foundDeviceCallback:\n");

    std::string value;
    std::string values[] = {
        "di",   "Device ID        ",
        "n",    "Device name      ",
        "lcv",  "Spec version url ",
        "dmv",  "Data Model Model ",
    };

    for (unsigned int i = 0; i < sizeof(values) / sizeof(values[0]) ; i += 2) {
        if (rep.getValue(values[i], value)) {
            DEBUG_MSG("\t%s:%s\n", values[i + 1].c_str(), value.c_str());
        }
    }

    PrintfOcRepresentation(rep);

    if (rep.getValue("di", value)) {
        std::string deviceUUID = value;

        IotivityDeviceInfo *deviceInfo = NULL;
        std::map<std::string, IotivityDeviceInfo *>::const_iterator it;
        if ((it = m_founddevicemap.find(deviceUUID)) != m_founddevicemap.end()) {
            deviceInfo = m_founddevicemap[deviceUUID];
        } else {
            deviceInfo = new IotivityDeviceInfo();

            deviceInfo->m_deviceinfomap["uuid"] = deviceUUID;

            if (rep.getValue("n", value))
                deviceInfo->m_deviceinfomap["name"] = value;
            if (rep.getValue("lcv", value))
                deviceInfo->m_deviceinfomap["coreSpecVersion"] = value;
            if (rep.getValue("dmv", value))
                deviceInfo->m_deviceinfomap["dataModels"] = value;
            m_founddevicemap[deviceUUID] = deviceInfo;
        }
    }
    
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

    DEBUG_MSG("handleFindDevices: device = %s\n"
              "\tresource = %s\n"
              "\tresourceType = %s\n"
              "\ttimeout = %d\n",
              deviceId.c_str(), resourceId.c_str(),
              resourceType.c_str(), waitsec);

    for (auto const &entity : m_founddevicemap) {
        IotivityDeviceInfo *deviceInfo = entity.second;
        delete deviceInfo;
    }
    m_founddevicemap.clear();

    OCConnectivityType connectivityType = CT_ADAPTER_IP;

    std::string hostUri = "";  //  multicast
    std::string platformDiscoveryURI   = "/oic/p";
    std::string platformDiscoveryRequest = OC_MULTICAST_PREFIX
                                       + platformDiscoveryURI;
    std::string deviceDiscoveryURI   = "/oic/d";
    std::string deviceDiscoveryRequest = OC_MULTICAST_PREFIX
                                       + deviceDiscoveryURI;

    DEBUG_MSG("process: hostUri=%s, uri1=%s, uri2=%s, timeout=%ds\n",
              hostUri.c_str(),
              platformDiscoveryRequest.c_str(),
              deviceDiscoveryRequest.c_str(),
              waitsec);

    FindPlatformCallback platformInfoHandler =
        std::bind(&IotivityClient::foundPlatformCallback,
                  this,
                  std::placeholders::_1);
    OCStackResult result = OCPlatform::getPlatformInfo(hostUri,
                           platformDiscoveryRequest,
                           connectivityType,
                           platformInfoHandler);
    if (OC_STACK_OK != result) {
        ERROR_MSG("OCPlatform::getPlatformInfo was unsuccessful\n");
        m_device->postError(async_call_id);
        return;
    }

    FindDeviceCallback deviceInfoHandler =
        std::bind(&IotivityClient::foundDeviceCallback,
                  this,
                  std::placeholders::_1,
                  waitsec);
    result = OCPlatform::getDeviceInfo(hostUri,
                           deviceDiscoveryRequest,
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

    DEBUG_MSG("getDeviceInfo: DONE2\n");
}

void IotivityClient::findDeviceTimerCallback(int waitsec) {
    sleep(waitsec);
    findDevicePreparedRequest();
}

void IotivityClient::findDevicePreparedRequest() {

    DEBUG_MSG("findDevicePreparedRequest\n");

    std::lock_guard<std::mutex> lock(m_callbackLockDevices);
    picojson::value::object object;
    object["cmd"] = picojson::value("foundDeviceCallback");
    object["asyncCallId"] = picojson::value(m_asyncCallId_finddevices);
    m_asyncCallId_finddevices = 0;
    picojson::array devicesArray;

    for (auto const &entity : m_founddevicemap) {
        IotivityDeviceInfo *deviceInfo = entity.second;
        picojson::object deviceobj;
        picojson::object properties;
        deviceInfo->serialize(properties);
        deviceobj["info"] = picojson::value(properties);
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
    std::string resId = value.get("id").to_str();
    IotivityResourceClient *resClient = getResourceById(resId);

    if (resClient != NULL) {
        OCStackResult result = resClient->createResource(oicResourceInit,
                                                         async_call_id);
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
    std::string resId = value.get("id").to_str();
    IotivityResourceClient *resClient = getResourceById(resId);

    if (resClient != NULL) {
        OCStackResult result = resClient->retrieveResource(async_call_id);
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
    std::string resId = param.get("id").to_str();
    IotivityResourceClient *resClient = getResourceById(resId);

    if (resClient != NULL) {
        IotivityResourceInit oicResourceInit(param);

        OCStackResult result =
        resClient->updateResource(oicResourceInit.m_resourceRep, async_call_id);

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
    std::string resId = value.get("id").to_str();
    IotivityResourceClient *resClient = getResourceById(resId);

    if (resClient != NULL) {
        OCStackResult result = resClient->deleteResource(async_call_id);
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
    std::string resId = value.get("id").to_str();
    IotivityResourceClient *resClient = getResourceById(resId);

    if (resClient != NULL) {
        result = resClient->startObserving(async_call_id);
        if (OC_STACK_OK != result) {
            m_device->postError(async_call_id);
            return;
        }
    } else {
        m_device->postError(async_call_id);
        return;
    }
}

void IotivityClient::handleCancelObserving(const picojson::value& value) {
    DEBUG_MSG("handleCancelObserving: v=%s\n", value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    std::string resId = value.get("id").to_str();
    IotivityResourceClient *resClient = getResourceById(resId);

    if (resClient != NULL) {
        OCStackResult result = resClient->cancelObserving(async_call_id);
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


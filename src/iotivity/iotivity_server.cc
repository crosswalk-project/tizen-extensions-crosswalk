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
#include "iotivity/iotivity_server.h"
#include "iotivity/iotivity_device.h"
#include "iotivity/iotivity_resource.h"

IotivityServer::IotivityServer(IotivityDevice* device) : m_device(device) {}

IotivityServer::~IotivityServer() {}

void *IotivityServer::getResourceById(std::string id) {
    if (m_resourcemap.size()) {
        std::map<std::string, void *>::const_iterator it;
        if ((it = m_resourcemap.find(id)) != m_resourcemap.end())
            return reinterpret_cast<void *>((*it).second);
    }

    return NULL;
}


void IotivityServer::handleRegisterResource(const picojson::value& value) {
    DEBUG_MSG("handleRegisterResource: v=%s\n", value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    IotivityResourceInit *resInit =
        new IotivityResourceInit(value.get("OicResourceInit"));
    IotivityResourceServer *resServer =
        new IotivityResourceServer(m_device, resInit);
    OCStackResult result = resServer->registerResource();

    if (OC_STACK_OK != result) {
        m_device->postError(async_call_id);
        return;
    }

    std::string resourceId = resServer->getResourceId();
    m_resourcemap[resourceId] = reinterpret_cast<void *>(resServer);
    picojson::value::object object;
    object["cmd"] = picojson::value("registerResourceCompleted");
    object["asyncCallId"] = picojson::value(async_call_id);
    resServer->serialize(object);
    picojson::value postvalue(object);
    m_device->PostMessage(postvalue.serialize().c_str());
}

void IotivityServer::handleUnregisterResource(const picojson::value& value) {
    DEBUG_MSG("handleUnregisterResource: v=%s\n", value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    std::string resId = value.get("resourceId").to_str();

    // Find and delete IotivityResourceServer *oicResourceServer
    IotivityResourceServer *resServer =
        reinterpret_cast<IotivityResourceServer *>(getResourceById(resId));

    if (resServer == NULL) {
        ERROR_MSG("handleUnregisterResource, resource not found\n");
        m_device->postError(async_call_id);
        return;
    }

    OCResourceHandle resHandle = reinterpret_cast<OCResourceHandle>
                                 (resServer->getResourceHandleToInt());
    OCStackResult result = OCPlatform::unregisterResource(resHandle);

    if (OC_STACK_OK != result) {
        ERROR_MSG("OCPlatform::unregisterResource was unsuccessful\n");
        m_device->postError(async_call_id);
        return;
    }

    if (resServer != NULL) {
        m_resourcemap.erase(resId);
        delete resServer;
    }

    m_device->postResult("unregisterResourceCompleted", async_call_id);
}

void IotivityServer::handleEnablePresence(const picojson::value& value) {
    DEBUG_MSG("handleEnablePresence: v=%s\n", value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    unsigned int ttl = 0;  // default
    OCStackResult result = OCPlatform::startPresence(ttl);

    if (OC_STACK_OK != result) {
        ERROR_MSG("OCPlatform::startPresence was unsuccessful\n");
        m_device->postError(async_call_id);
        return;
    }

    m_device->postResult("enablePresenceCompleted", async_call_id);
}

void IotivityServer::handleDisablePresence(const picojson::value& value) {
    DEBUG_MSG("handleDisablePresence: v=%s\n", value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    OCStackResult result = OCPlatform::stopPresence();

    if (OC_STACK_OK != result) {
        ERROR_MSG("OCPlatform::stopPresence was unsuccessful\n");
        m_device->postError(async_call_id);
        return;
    }

    m_device->postResult("disablePresenceCompleted", async_call_id);
}

void IotivityServer::handleNotify(const picojson::value& value) {
    DEBUG_MSG("handleNotify: v=%s\n", value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    std::string resId = value.get("resourceId").to_str();
    std::string method = value.get("method").to_str();
    picojson::value updatedPropertyNames = value.get("updatedPropertyNames");

    IotivityResourceServer *resServer =
    reinterpret_cast<IotivityResourceServer *>(getResourceById(resId));

    if (resServer == NULL) {
        ERROR_MSG("handleNotify, resource not found was unsuccessful\n");
        m_device->postError(async_call_id);
        return;
    }

    if (method == "update") {
        auto pResponse = std::make_shared<OC::OCResourceResponse>();
        pResponse->setErrorCode(200);
        pResponse->setResourceRepresentation(
                      resServer->getRepresentation(),
                      DEFAULT_INTERFACE);

        OCResourceHandle resHandle = reinterpret_cast<OCResourceHandle>
                                     (resServer->getResourceHandleToInt());

        ObservationIds& observationIds = resServer->getObserversList();
        OCStackResult result = OCPlatform::notifyListOfObservers(
                               resHandle,
                               observationIds,
                               pResponse);
        if (OC_STACK_OK != result) {
            ERROR_MSG("OCPlatform::notifyAllObservers was unsuccessful\n");
            m_device->postError(async_call_id);
            return;
        }
    }

    m_device->postResult("notifyCompleted", async_call_id);
}


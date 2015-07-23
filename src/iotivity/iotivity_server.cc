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

void *IotivityServer::getResourceById(int id) {
    if (m_resourcemap.size()) {
        std::map<int, void *>::const_iterator it;
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

    int index = resServer->getResourceHandleToInt();
    m_resourcemap[index] = reinterpret_cast<void *>(resServer);
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
    int resId = static_cast<int>(value.get("resourceId").get<double>());
    OCResourceHandle resHandle = (OCResourceHandle)(resId);
    OCStackResult result = OCPlatform::unregisterResource(resHandle);

    if (OC_STACK_OK != result) {
        std::cerr << "OCPlatform::unregisterResource was unsuccessful\n";
        m_device->postError(async_call_id);
        return;
    }

    // Find and delete IotivityResourceServer *oicResourceServer
    IotivityResourceServer *resServer =
        reinterpret_cast<IotivityResourceServer *>(getResourceById(resId));

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
        std::cerr << "OCPlatform::startPresence was unsuccessful\n";
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
        std::cerr << "OCPlatform::stopPresence was unsuccessful\n";
        m_device->postError(async_call_id);
        return;
    }

    m_device->postResult("disablePresenceCompleted", async_call_id);
}

void IotivityServer::handleNotify(const picojson::value& value) {
    DEBUG_MSG("handleNotify: v=%s\n", value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    int resourceId = static_cast<int>(value.get("resourceId").get<double>());
    std::string method = value.get("method").to_str();
    picojson::value updatedPropertyNames = value.get("updatedPropertyNames");
    OCStackResult result = OCPlatform::notifyAllObservers(
                               (OCResourceHandle)resourceId);

    if (OC_STACK_OK != result) {
        std::cerr << "OCPlatform::notifyAllObservers was unsuccessful\n";
        m_device->postError(async_call_id);
        return;
    }

    m_device->postResult("notifyCompleted", async_call_id);
}


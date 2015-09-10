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
#include "iotivity/iotivity_server.h"
#include "iotivity/iotivity_device.h"
#include "iotivity/iotivity_resource.h"

IotivityServer::IotivityServer(IotivityDevice* device) : m_device(device) {}

IotivityServer::~IotivityServer() {}

IotivityResourceServer* IotivityServer::getResourceById(std::string id) {
  if (m_resourcemap.size()) {
    std::map<std::string, IotivityResourceServer*>::const_iterator it;
    if ((it = m_resourcemap.find(id)) != m_resourcemap.end())
      return (*it).second;
  }

  return NULL;
}

void IotivityServer::handleRegisterResource(const picojson::value& value) {
  DEBUG_MSG("handleRegisterResource: v=%s\n", value.serialize().c_str());

  double async_call_id = value.get("asyncCallId").get<double>();
  IotivityResourceInit* resInit =
      new IotivityResourceInit(value.get("OicResourceInit"));
  IotivityResourceServer* resServer =
      new IotivityResourceServer(m_device, resInit);
  OCStackResult result = resServer->registerResource();

  if (OC_STACK_OK != result) {
    m_device->postError(async_call_id);
    return;
  }

  std::string resourceId = resServer->getResourceId();
  m_resourcemap[resourceId] = resServer;
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
  IotivityResourceServer* resServer = getResourceById(resId);

  if (resServer == NULL) {
    ERROR_MSG("handleUnregisterResource, resource not found\n");
    m_device->postError(async_call_id);
    return;
  }

  OCResourceHandle resHandle =
      reinterpret_cast<OCResourceHandle>(resServer->getResourceHandleToInt());
  OCStackResult result = OCPlatform::unregisterResource(resHandle);

  if (OC_STACK_OK != result) {
    ERROR_MSG("OCPlatform::unregisterResource was unsuccessful\n");
    m_device->postError(async_call_id);
    return;
  }

  m_resourcemap.erase(resId);
  delete resServer;

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

  IotivityResourceServer* resServer = getResourceById(resId);

  if (resServer == NULL) {
    ERROR_MSG("handleNotify, resource not found was unsuccessful\n");
    m_device->postError(async_call_id);
    return;
  }

  if (method == "update") {
    auto pResponse = std::make_shared<OC::OCResourceResponse>();
    pResponse->setErrorCode(200);
    pResponse->setResourceRepresentation(resServer->getRepresentation(),
                                         DEFAULT_INTERFACE);

    OCResourceHandle resHandle =
        reinterpret_cast<OCResourceHandle>(resServer->getResourceHandleToInt());

    ObservationIds& observationIds = resServer->getObserversList();
    OCStackResult result =
        OCPlatform::notifyListOfObservers(resHandle, observationIds, pResponse);
    if (OC_STACK_OK != result) {
      ERROR_MSG("OCPlatform::notifyAllObservers was unsuccessful\n");
      m_device->postError(async_call_id);
      return;
    }
  }

  m_device->postResult("notifyCompleted", async_call_id);
}

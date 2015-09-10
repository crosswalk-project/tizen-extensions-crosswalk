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

#include "iotivity/iotivity_instance.h"
#include "iotivity/iotivity_device.h"
#include "iotivity/iotivity_server.h"
#include "iotivity/iotivity_client.h"
#include "iotivity/iotivity_resource.h"

std::map<int, OCRepresentation> ResourcesMap;

IotivityInstance::IotivityInstance() {
  pDebugEnv = getenv("IOTIVITY_DEBUG");
  m_device = new IotivityDevice(this, NULL);
}

IotivityInstance::~IotivityInstance() { delete m_device; }

void IotivityInstance::HandleMessage(const char* msg) {
  if (pDebugEnv != NULL)
    printf("\n\n[JS==>Native] IotivityInstance::HandleMessage: %s\n", msg);

  picojson::value v;
  std::string error;

  picojson::parse(v, msg, msg + strlen(msg), &error);
  if (!error.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }

  std::string cmd = v.get("cmd").to_str();

  // Device
  if (cmd == "configure")
    m_device->handleConfigure(v);
  else if (cmd == "factoryReset")
    m_device->handleFactoryReset(v);
  else if (cmd == "reboot")
    m_device->handleReboot(v);
  // Client
  else if (cmd == "findResources")
    m_device->getClient()->handleFindResources(v);
  else if (cmd == "findDevices")
    m_device->getClient()->handleFindDevices(v);
  else if (cmd == "createResource")
    m_device->getClient()->handleCreateResource(v);
  else if (cmd == "retrieveResource")
    m_device->getClient()->handleRetrieveResource(v);
  else if (cmd == "updateResource")
    m_device->getClient()->handleUpdateResource(v);
  else if (cmd == "deleteResource")
    m_device->getClient()->handleDeleteResource(v);
  else if (cmd == "startObserving")
    m_device->getClient()->handleStartObserving(v);
  else if (cmd == "cancelObserving")
    m_device->getClient()->handleCancelObserving(v);
  // Server
  else if (cmd == "registerResource")
    m_device->getServer()->handleRegisterResource(v);
  else if (cmd == "unregisterResource")
    m_device->getServer()->handleUnregisterResource(v);
  else if (cmd == "enablePresence")
    m_device->getServer()->handleEnablePresence(v);
  else if (cmd == "disablePresence")
    m_device->getServer()->handleDisablePresence(v);
  else if (cmd == "notify")
    m_device->getServer()->handleNotify(v);
  else if (cmd == "sendResponse")
    handleSendResponse(v);
  else if (cmd == "sendError")
    handleSendError(v);
  else
    ERROR_MSG(std::string("Received unknown message: " + cmd + "\n").c_str());

  return;
}

void IotivityInstance::handleSendResponse(const picojson::value& value) {
  DEBUG_MSG("handleSendResponse: v=%s\n", value.serialize().c_str());

  double async_call_id = value.get("asyncCallId").get<double>();

  picojson::value resource = value.get("resource");
  picojson::value OicRequestEvent = value.get("OicRequestEvent");
  IotivityRequestEvent iotivityRequestEvent;
  iotivityRequestEvent.deserialize(OicRequestEvent);
  OCStackResult result = iotivityRequestEvent.sendResponse();

  if (OC_STACK_OK != result) {
    m_device->postError(async_call_id);
    return;
  }

  m_device->postResult("sendResponseCompleted", async_call_id);
}

void IotivityInstance::handleSendError(const picojson::value& value) {
  DEBUG_MSG("handleSendError: v=%s\n", value.serialize().c_str());

  double async_call_id = value.get("asyncCallId").get<double>();
  std::string errorMsg = value.get("error").to_str();
  picojson::value OicRequestEvent = value.get("OicRequestEvent");
  IotivityRequestEvent iotivityRequestEvent;
  iotivityRequestEvent.deserialize(OicRequestEvent);
  OCStackResult result = iotivityRequestEvent.sendError();

  if (OC_STACK_OK != result) {
    m_device->postError(async_call_id);
    return;
  }

  m_device->postResult("sendResponseCompleted", async_call_id);
}

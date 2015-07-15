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
#include "iotivity/iotivity_instance.h"
#include "iotivity/iotivity_device.h"
#include "iotivity/iotivity_server.h"
#include "iotivity/iotivity_client.h"


std::map<int, OCRepresentation> ResourcesMap;



IotivityInstance::IotivityInstance() {
    pDebugEnv = getenv("IOTIVITY_DEBUG");
    m_device = new IotivityDevice(this);
}

IotivityInstance::~IotivityInstance() {

    delete m_device;
}

void IotivityInstance::HandleMessage(const char* message) {
  std::string resp = PrepareMessage(message);
  //PostMessage(resp.c_str());  // to javascript extension.setMessageListener()
}

void IotivityInstance::HandleSyncMessage(const char* message) {
  //std::string resp = PrepareMessage(message);
  //SendSyncReply(resp.c_str());
}

std::string IotivityInstance::PrepareMessage(const std::string & message) {

  const char *msg = message.c_str();
  std::string resp = "";

  if (pDebugEnv != NULL)
    printf("IotivityInstance::PrepareMessage: %s\n", msg);

  picojson::value v;
  std::string error;

  picojson::parse(v, msg, msg + strlen(msg), &error);
  if (!error.empty()) {
    std::cout << "Ignoring message.\n";
    return resp;
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
    std::cerr << "Received unknown message: " << cmd << "\n";

  return resp;
}


void IotivityInstance::handleSendResponse(const picojson::value& value) {

    DEBUG_MSG("handleSendResponse: v=%s\n",value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();  
    std::string requestType = value.get("type").to_str();
    int requestId = (int)value.get("requestId").get<double>();
    int source = (int)value.get("source").get<double>();
    int target = (int)value.get("target").get<double>();

    picojson::value resource = value.get("resource");

    DEBUG_MSG("handleSendResponse: requestHandle=0x%x\n", requestId);

    auto pResponse = std::make_shared<OC::OCResourceResponse>();
    pResponse->setRequestHandle((void *)requestId);
    pResponse->setResourceHandle((void *)target);


    OCRepresentation oCRepresentation; // = request->getResourceRepresentation();
    oCRepresentation.setUri("/a/light");
    oCRepresentation.setValue("state", "true");
    oCRepresentation.setValue("power", 10);

    //pResponse->setHeaderOptions(serverHeaderOptions);

    DEBUG_MSG("handleSendResponse: type=%s\n", requestType.c_str());
    if (requestType == "retrieve")
    {
        pResponse->setResourceRepresentation(oCRepresentation);
        pResponse->setErrorCode(200);
        pResponse->setResponseResult(OC_EH_OK);
    }
    else if (requestType == "update")
    {
        pResponse->setResourceRepresentation(oCRepresentation);
        pResponse->setErrorCode(200);
        pResponse->setResponseResult(OC_EH_OK);
    }
    else if (requestType == "create") // POST (first time)
    {
        //OCRepresentation rep = pRequest->getResourceRepresentation();
        //OCRepresentation rep_post = post(rep);
        pResponse->setResourceRepresentation(oCRepresentation);
        pResponse->setErrorCode(200);
        pResponse->setResponseResult(OC_EH_OK);
    }
    else if (requestType == "observe")
    {
        DEBUG_MSG("handleSendResponse: ObserverFlag\n");
    }

    OCStackResult result = OCPlatform::sendResponse(pResponse);
    if (OC_STACK_OK != result)
    {
        std::cerr << "OCPlatform::sendResponse was unsuccessful\n";
        m_device->postError(async_call_id);
        return;
    }

    m_device->postResult("sendResponseCompleted", async_call_id);
}

void IotivityInstance::handleSendError(const picojson::value& value) {

    DEBUG_MSG("handleSendError: v=%s\n",value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    std::string errorMsg = value.get("error").to_str();
    int requestId = (int)value.get("requestId").get<double>();
    int source = (int)value.get("source").get<double>();
    int target = (int)value.get("target").get<double>();

    DEBUG_MSG("handleSendError: requestHandle=0x%x\n", requestId);

    auto pResponse = std::make_shared<OC::OCResourceResponse>();
    pResponse->setRequestHandle((void *)requestId);
    pResponse->setResourceHandle((void *)target);

    pResponse->setErrorCode(200 /* TODOvalue.get("error").to_str()*/);
    pResponse->setResponseResult(OC_EH_OK);

    OCStackResult result = OCPlatform::sendResponse(pResponse);
    if (OC_STACK_OK != result)
    {
        std::cerr << "OCPlatform::sendResponse (Error) was unsuccessful\n";
        m_device->postError(async_call_id);
        return;
    }

    DEBUG_MSG("handleSendError:4\n");
    m_device->postResult("sendResponseCompleted", async_call_id);
}





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


#include <stdio.h>
#include <stdlib.h>
#include <string>


char *pDebugEnv = NULL;


IotivityInstance::IotivityInstance() {
    pDebugEnv = getenv("IOTIVITY_DEBUG");
}

IotivityInstance::~IotivityInstance() {
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

  if (cmd == "factoryReset")
    handleFactoryReset(v);
  else if (cmd == "reboot")
    handleReboot(v);
 else if (cmd == "findResources")
    handleFindResources(v);
  else if (cmd == "findDevices")
    handleFindDevices(v);
  else if (cmd == "createResource")
    handleCreateResource(v);
  else if (cmd == "retrieveResource")
    handleRetrieveResource(v);
  else if (cmd == "updateResource")
    handleUpdateResource(v);
  else if (cmd == "deleteResource")
    handleDeleteResource(v);
  else if (cmd == "startObserving")
    handleStartObserving(v);
  else if (cmd == "cancelObserving")
    handleCancelObserving(v);
  else if (cmd == "registerResource")
    handleRegisterResource(v);
  else if (cmd == "unregisterResource")
    handleUnregisterResource(v);
  else if (cmd == "enablePresence")
    handleEnablePresence(v);
  else if (cmd == "disablePresence")
    handleDisablePresence(v);
  else if (cmd == "notify")
    handleNotify(v);
  else if (cmd == "sendResponse")
    handleSendResponse(v);
  else if (cmd == "sendError")
    handleSendError(v);

  else
    std::cerr << "Received unknown message: " << cmd << "\n";

  return resp;
}


void IotivityInstance::handleFactoryReset(const picojson::value& value) {

    double async_call_id = value.get("asyncCallId").get<double>();
    picojson::value param = value.get("OicResourceInit");

}

void IotivityInstance::handleReboot(const picojson::value& value) {

    double async_call_id = value.get("asyncCallId").get<double>();
    picojson::value param = value.get("OicResourceInit");

}

void IotivityInstance::foundResourceCallback(std::shared_ptr<OCResource> resource)
{


}


void IotivityInstance::handleFindResources(const picojson::value& value) {

    double async_call_id = value.get("asyncCallId").get<double>();
    picojson::value param = value.get("OicDiscoveryOptions");

    //std::string deviceId = param.get("deviceId").to_str();
    //std::string resourceId = param.get("resourceId").to_str();
    std::string resourceType = param.get("resourceType").to_str();
 
    FindCallback resourceHandler = std::bind(&IotivityInstance::foundResourceCallback, 
                                             this, 
                                             std::placeholders::_1);
    OCStackResult result = OCPlatform::findResource("", 
                                                    resourceType,
                                                    OC_ALL, 
                                                    resourceHandler);
    if (OC_STACK_OK != result)
    {
        std::cerr << "OCPlatform::findResource was unsuccessful\n";
        postError(async_call_id);
        return;
    }
}

void IotivityInstance::foundDeviceCallback(const OCRepresentation& rep) {


}

void IotivityInstance::handleFindDevices(const picojson::value& value) {

    double async_call_id = value.get("asyncCallId").get<double>();
    picojson::value param = value.get("OicDiscoveryOptions");

    std::string deviceId = param.get("deviceId").to_str();
    std::string resourceId = param.get("resourceId").to_str();
    //std::string resourceType = param.get("resourceType").to_str();
 
    OCConnectivityType connectivityType = OC_IPV4;
    FindDeviceCallback deviceInfoHandler = std::bind(&IotivityInstance::foundDeviceCallback, 
                                                    this, 
                                                    std::placeholders::_1);
    OCStackResult result = OCPlatform::getDeviceInfo(deviceId, 
                                                     resourceId,
                                                     connectivityType, 
                                                     deviceInfoHandler);
    if (OC_STACK_OK != result)
    {
        std::cerr << "OCPlatform::getDeviceInfo was unsuccessful\n";
        postError(async_call_id);
        return;
    }
}

void IotivityInstance::handleCreateResource(const picojson::value& value) {
    double async_call_id = value.get("asyncCallId").get<double>();
    picojson::value param = value.get("OicResourceInit");
}

void IotivityInstance::handleRetrieveResource(const picojson::value& value) {
    double async_call_id = value.get("asyncCallId").get<double>();
    picojson::value param = value.get("OicResourceInit");
}

void IotivityInstance::handleUpdateResource(const picojson::value& value) {
    double async_call_id = value.get("asyncCallId").get<double>();
    picojson::value param = value.get("OicResourceInit");

}

void IotivityInstance::handleDeleteResource(const picojson::value& value) {

    double async_call_id = value.get("asyncCallId").get<double>();
    picojson::value param = value.get("OicResourceInit");

}

void IotivityInstance::handleStartObserving(const picojson::value& value) {

    double async_call_id = value.get("asyncCallId").get<double>();
    picojson::value param = value.get("OicResourceInit");

}

void IotivityInstance::handleCancelObserving(const picojson::value& value) {

    double async_call_id = value.get("asyncCallId").get<double>();
    picojson::value param = value.get("OicResourceInit");

}

void IotivityInstance::postEntityHandler(std::shared_ptr<OCResourceRequest> request) {

  picojson::value::object object;
  object["cmd"] = picojson::value("entityHandler");
  //object["OicRequestEvent"] = picojson::value(request); TODO

  picojson::value value(object);
  PostMessage(value.serialize().c_str());
}

OCEntityHandlerResult IotivityInstance::entityHandlerCallback(std::shared_ptr<OCResourceRequest> request) {

    cout << "\tIn entityHandlerCallback:\n";
    OCEntityHandlerResult ehResult = OC_EH_ERROR;
    if(request)
    {
        postEntityHandler(request);
    }
    else
    {
        std::cerr << "entityHandlerCallback: Request invalid" << std::endl;
        //postError(async_call_id);
    }

    return ehResult;
}

void IotivityInstance::postResult(const char* completed_operation,
                                  double async_operation_id) {
  picojson::value::object object;
  object["cmd"] = picojson::value(completed_operation);
  object["asyncCallId"] = picojson::value(async_operation_id);

  picojson::value value(object);
  PostMessage(value.serialize().c_str());
}

void IotivityInstance::postError(double async_operation_id) {
  picojson::value::object object;
  object["cmd"] = picojson::value("asyncCallError");
  object["asyncCallId"] = picojson::value(async_operation_id);

  picojson::value value(object);
  PostMessage(value.serialize().c_str());
}

void IotivityInstance::postRegisterResource(double async_operation_id, OCResourceHandle resHandle) {

  picojson::value::object object;
  object["cmd"] = picojson::value("registerResourceCompleted");
  object["asyncCallId"] = picojson::value(async_operation_id);

  //object["OicResourceInit"] =  TODO
  object["resourceId"] = picojson::value((double)((int)resHandle));

  picojson::value value(object);
  PostMessage(value.serialize().c_str());
}

void IotivityInstance::handleRegisterResource(const picojson::value& value) {

    double async_call_id = value.get("asyncCallId").get<double>();
    picojson::value param = value.get("OicResourceInit");

    std::string deviceId = param.get("deviceId").to_str();
    std::string connectionMode = param.get("connectionMode").to_str();
    picojson::array resourceTypes = param.get("resourceTypes").get<picojson::array>();
    picojson::array interfaces = param.get("interfaces").get<picojson::array>();
    bool discoverable = param.get("discoverable").get<bool>();
    bool observable = param.get("observable").get<bool>();
    bool isSecure = false;

    OCResourceHandle resHandle;
    std::string resourceURI = param.get("url").to_str();
    std::string resourceTypeName = "";
    std::string resourceInterface = DEFAULT_INTERFACE;
    uint8_t resourceProperty = 0;

    for (picojson::array::iterator iter = resourceTypes.begin(); iter != resourceTypes.end(); ++iter) {
        printf("array resourceTypes value =%s\n", (*iter).get<string>().c_str());
        if (resourceTypeName == "")
        {
            resourceTypeName = (*iter).get<string>();
        }
    }

    for (picojson::array::iterator iter = interfaces.begin(); iter != interfaces.end(); ++iter) {
        printf("array interfaces value =%s\n", (*iter).get<string>().c_str());
        if (resourceInterface == "")
        {
            resourceInterface = (*iter).get<string>();
        }
    }

    if (discoverable)
        resourceProperty |= OC_DISCOVERABLE;

    if (observable)
        resourceProperty |= OC_OBSERVABLE;

    if (isSecure)
        resourceProperty |= OC_SECURE;


    EntityHandler resourceCallback = std::bind(&IotivityInstance::entityHandlerCallback, 
                                               this, 
                                               std::placeholders::_1);

    OCStackResult result = OCPlatform::registerResource(
                                       resHandle, 
                                       resourceURI, 
                                       resourceTypeName,
                                       resourceInterface, 
                                       resourceCallback, 
                                       resourceProperty);
    if (OC_STACK_OK != result)
    {
        std::cerr << "OCPlatform::registerResource was unsuccessful\n";
        postError(async_call_id);
        return;
    }

    postRegisterResource(async_call_id, resHandle);
}

void IotivityInstance::handleUnregisterResource(const picojson::value& value) {

    double async_call_id = value.get("asyncCallId").get<double>();
    int resHandleInt = (int)value.get("resourceId").get<double>();
    OCResourceHandle resHandle = (OCResourceHandle)(resHandleInt);

    OCStackResult result = OCPlatform::unregisterResource(resHandle);
    if (OC_STACK_OK != result)
    {
        std::cerr << "OCPlatform::unregisterResource was unsuccessful\n";
        postError(async_call_id);
        return;
    }

    postResult("unregisterResourceCompleted", async_call_id);
}

void IotivityInstance::handleEnablePresence(const picojson::value& value) {

    double async_call_id = value.get("asyncCallId").get<double>();

    unsigned int ttl = 0; // default
    OCStackResult result = OCPlatform::startPresence(ttl);
    if (OC_STACK_OK != result)
    {
        std::cerr << "OCPlatform::startPresence was unsuccessful\n";
        postError(async_call_id);
        return;
    }

    postResult("enablePresenceCompleted", async_call_id);
}

void IotivityInstance::handleDisablePresence(const picojson::value& value) {

    double async_call_id = value.get("asyncCallId").get<double>();

    OCStackResult result = OCPlatform::stopPresence();
    if (OC_STACK_OK != result)
    {
        std::cerr << "OCPlatform::stopPresence was unsuccessful\n";
        postError(async_call_id);
        return;
    }

    postResult("disablePresenceCompleted", async_call_id);
}

void IotivityInstance::handleNotify(const picojson::value& value) {

}


void IotivityInstance::handleSendResponse(const picojson::value& value) {

//OCStackResult sendResponse(const std::shared_ptr<OCResourceResponse> pResponse);
}

void IotivityInstance::handleSendError(const picojson::value& value) {

}





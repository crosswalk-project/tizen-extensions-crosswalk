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
const int SUCCESS_RESPONSE = 0;


#define INFO_MSG(msg, ...) { printf(msg, ##__VA_ARGS__);}
#define DEBUG_MSG(msg, ...) { if (pDebugEnv) printf(msg, ##__VA_ARGS__);}


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

  if (cmd == "configure")
    handleConfigure(v);
  else if (cmd == "factoryReset")
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

static void DuplicateString(char ** targetString, std::string sourceString) {
    *targetString = new char[sourceString.length() + 1];
    strncpy(*targetString, sourceString.c_str(), (sourceString.length() + 1));
}

 void DeletePlatformInfo(OCPlatformInfo &platformInfo) {
    delete[] platformInfo.platformID;
    delete[] platformInfo.manufacturerName;
    delete[] platformInfo.manufacturerUrl;
    delete[] platformInfo.modelNumber;
    delete[] platformInfo.dateOfManufacture;
    delete[] platformInfo.platformVersion;
    delete[] platformInfo.operatingSystemVersion;
    delete[] platformInfo.hardwareVersion;
    delete[] platformInfo.firmwareVersion;
    delete[] platformInfo.supportUrl;
    delete[] platformInfo.systemTime;
}

static OCStackResult SetPlatformInfo(OCPlatformInfo &platformInfo, std::string platformID, std::string manufacturerName,
    std::string manufacturerUrl, std::string modelNumber, std::string dateOfManufacture,
    std::string platformVersion, std::string operatingSystemVersion, std::string hardwareVersion,
    std::string firmwareVersion, std::string supportUrl, std::string systemTime) {
    DuplicateString(&platformInfo.platformID, platformID);
    DuplicateString(&platformInfo.manufacturerName, manufacturerName);
    DuplicateString(&platformInfo.manufacturerUrl, manufacturerUrl);
    DuplicateString(&platformInfo.modelNumber, modelNumber);
    DuplicateString(&platformInfo.dateOfManufacture, dateOfManufacture);
    DuplicateString(&platformInfo.platformVersion, platformVersion);
    DuplicateString(&platformInfo.operatingSystemVersion, operatingSystemVersion);
    DuplicateString(&platformInfo.hardwareVersion, hardwareVersion);
    DuplicateString(&platformInfo.firmwareVersion, firmwareVersion);
    DuplicateString(&platformInfo.supportUrl, supportUrl);
    DuplicateString(&platformInfo.systemTime, systemTime);
    return OC_STACK_OK;
}

void IotivityInstance::handleConfigure(const picojson::value& value) {

    double async_call_id = value.get("asyncCallId").get<double>();
    std::string deviceRole = value.get("settings").get("role").to_str();
    picojson::value param = value.get("settings").get("info");

    std::string platformID = param.get("uuid").to_str();
    std::string manufacturerName = param.get("manufacturerName").to_str();
    std::string manufacturerUrl = param.get("manufacturerUrl").to_str();
    std::string modelNumber = param.get("model").to_str();
    std::string manufactureDate = param.get("manufactureDate").to_str();
    std::string platformVersion = param.get("coreSpecVersion").to_str();
    std::string operatingSystemVersion = param.get("osVersion").to_str();
    std::string hardwareVersion = param.get("hardwareVersion").to_str();
    std::string firmwareVersion = param.get("firmwareVersion").to_str();
    std::string supportUrl = param.get("supportUrl").to_str();
    std::string systemTime = param.get("uuid").to_str();

    OCPlatformInfo platformInfo = {0};

    OCStackResult result = SetPlatformInfo(platformInfo,                                          
                                           platformID, 
                                           manufacturerName, 
                                           manufacturerUrl,
                                           modelNumber, 
                                           manufactureDate, 
                                           platformVersion, 
                                           operatingSystemVersion,
                                           hardwareVersion, 
                                           firmwareVersion, 
                                           supportUrl, 
                                           systemTime);
    if (OC_STACK_OK != result)
    {
        std::cerr << "Platform Registration was unsuccessful\n";
        postError(async_call_id);
        return;
    }

    result = OCPlatform::registerPlatformInfo(platformInfo);

    if (OC_STACK_OK != result)
    {
        std::cerr << "OCPlatform::registerPlatformInfo was unsuccessful\n";
        postError(async_call_id);
        return;
    }

    DeletePlatformInfo(platformInfo
);

    if (deviceRole == "client")
    {
        PlatformConfig cfg
        {
            ServiceType::InProc,
            ModeType::Client,
            "0.0.0.0",
            0,
            QualityOfService::LowQos
        };

        OCPlatform::Configure(cfg);
    }
    else if (deviceRole == "server")
    {
        PlatformConfig cfg
        {
            ServiceType::InProc,
            ModeType::Server,
            "0.0.0.0", // By setting to "0.0.0.0", it binds to all available interfaces
            0,         // Uses randomly available port
            QualityOfService::LowQos
        };

        OCPlatform::Configure(cfg);
    }
    else if (deviceRole == "intermediate")
    {

    }

    postResult("configureCompleted", async_call_id);
}

void IotivityInstance::handleFactoryReset(const picojson::value& value) {

    double async_call_id = value.get("asyncCallId").get<double>();

    // Return to factory configuration + reboot


}

void IotivityInstance::handleReboot(const picojson::value& value) {

    double async_call_id = value.get("asyncCallId").get<double>();

    // Keep current configuration + reboot

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


void IotivityInstance::onPut(const HeaderOptions& headerOptions, const OCRepresentation& rep, const int eCode) {

    if (eCode == SUCCESS_RESPONSE)
    {

    }
    else
    {

    }
}

void IotivityInstance::onGet(const HeaderOptions& headerOptions, const OCRepresentation& rep, const int eCode) {

    if (eCode == SUCCESS_RESPONSE)
    {

    }
    else
    {

    }
}

void IotivityInstance::onPost(const HeaderOptions& headerOptions, const OCRepresentation& rep, const int eCode) {

    if (eCode == SUCCESS_RESPONSE)
    {

    }
    else
    {

    }
}

void IotivityInstance::handleCRUDNRRequest(const picojson::value& value) {
    double async_call_id = value.get("asyncCallId").get<double>();
    picojson::value param = value.get("request");

    int resHandleInt = (int)value.get("request").get("target").get<double>();
    //OCResourceHandle resHandle = (OCResourceHandle)(resHandleInt);
    OCResource *resource = (OCResource *)(resHandleInt);


    OCResourceHandle resHandle;

    OCResourceRequest *request;
    
    OCRepresentation oCRepresentation = request->getResourceRepresentation();
    std::string requestType = request->getRequestType();

    if (requestType == "get")
    {
        //oCRepresentation.getValue("state", mylight.m_state);
        //oCRepresentation.getValue("power", mylight.m_power);
        //QueryParamsMap test;
        //resource->get(test, &onGet,OC::QualityOfService::HighQos);

    }
    else if (requestType == "put")
    {
        //oCRepresentation.setValue("state", mylight.m_state);
        //oCRepresentation.setValue("power", mylight.m_power);
        //resource->put(oCRepresentation, QueryParamsMap(), &onPut, OC::QualityOfService::HighQos);
    }
    else if (requestType == "post")
    {
       //resource->post(oCRepresentation, QueryParamsMap(), &onPost2, OC::QualityOfService::HighQos);
    }
    else if (requestType == "delete")
    {

    }

    
}

void IotivityInstance::handleCreateResource(const picojson::value& value) {
    handleCRUDNRRequest(value);
}

void IotivityInstance::handleRetrieveResource(const picojson::value& value) {
    handleCRUDNRRequest(value);
}

void IotivityInstance::handleUpdateResource(const picojson::value& value) {
    handleCRUDNRRequest(value);
}

void IotivityInstance::handleDeleteResource(const picojson::value& value) {
    handleCRUDNRRequest(value);
}

void IotivityInstance::handleStartObserving(const picojson::value& value) {
    handleCRUDNRRequest(value);
}

void IotivityInstance::handleCancelObserving(const picojson::value& value) {
    handleCRUDNRRequest(value);
}

void IotivityInstance::postEntityHandler(std::shared_ptr<OCResourceRequest> request) {

    picojson::value::object object;
    object["cmd"] = picojson::value("entityHandler");

    picojson::value::object requestObject;

    std::string requestType = request->getRequestType();
    int requestFlag = request->getRequestHandlerFlag();
    QueryParamsMap queries = request->getQueryParameters();


    if (!queries.empty())
    {
        std::cout << "\nQuery processing upto entityHandler" << std::endl;
    }
    for (auto it : queries)
    {
        std::cout << "Query key: " << it.first << " value : " << it.second
                        << std:: endl;
    }

    //requestObject["requestId"] = picojson::value(request->getRequestHandle());
    //requestObject["source"] = picojson::value(request->getRequestHandle());
    //requestObject["target"] = picojson::value(request->getResourceUri());


    if (requestType == "GET")
    {
        requestObject["type"] = picojson::value("retrieve");
    }
    else if (requestType == "PUT")
    {
        requestObject["type"] = picojson::value("update");

        OCRepresentation oCRepresentation = request->getResourceRepresentation();

        //updatedPropertyNames;
        //oCRepresentation.setValue("state", m_state);


        //requestObject["res"] = picojson::value(oCRepresentation);

    }
    else if (requestType == "POST")
    {
        requestObject["type"] = picojson::value("observe");

    }
    else if (requestType == "DELETE")
    {
        requestObject["type"] = picojson::value("delete");
    }
    else if (requestType == "CREATE")
    {
        requestObject["type"] = picojson::value("create");

        OCRepresentation oCRepresentation = request->getResourceRepresentation();
        //requestObject["res"] = picojson::value(oCRepresentation);
    }


    if (requestFlag & RequestHandlerFlag::ObserverFlag)
    {
        requestObject["type"] = picojson::value("observe");

        ObservationInfo observationInfo = request->getObservationInfo();
        if(ObserveAction::ObserveRegister == observationInfo.action)
        {
            //m_interestedObservers.push_back(observationInfo.obsId);
        }
        else if(ObserveAction::ObserveUnregister == observationInfo.action)
        {
/*
            m_interestedObservers.erase(std::remove(
                                                        m_interestedObservers.begin(),
                                                        m_interestedObservers.end(),
                                                        observationInfo.obsId),
                                                        m_interestedObservers.end());
*/
        }
    }
    else if  (requestFlag & RequestHandlerFlag::RequestFlag)
    {

    }

    //requestObject["queryOptions"] = picojson::value(queries);

    HeaderOptions headerOptions = request->getHeaderOptions();
    //requestObject["headerOptions"] = picojson::value(headerOptions);


    picojson::value value(object);
    PostMessage(value.serialize().c_str());
}

OCEntityHandlerResult IotivityInstance::entityHandlerCallback(std::shared_ptr<OCResourceRequest> request) {

    DEBUG_MSG("In entityHandlerCallback:\n");
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

void IotivityInstance::postRegisterResource(double async_operation_id, OCResourceHandle resHandle, const picojson::value& param) {

  picojson::value::object object;
  object["cmd"] = picojson::value("registerResourceCompleted");
  object["asyncCallId"] = picojson::value(async_operation_id);
  object["OicResourceInit"] = param;
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

    DEBUG_MSG("handleRegisterResource\n");

    for (picojson::array::iterator iter = resourceTypes.begin(); iter != resourceTypes.end(); ++iter) {
        DEBUG_MSG("array resourceTypes value=%s\n", (*iter).get<string>().c_str());
        if (resourceTypeName == "")
        {
            resourceTypeName = (*iter).get<string>();
            break;
        }
    }

    for (picojson::array::iterator iter = interfaces.begin(); iter != interfaces.end(); ++iter) {
        DEBUG_MSG("array interfaces value=%s\n", (*iter).get<string>().c_str());
        if (resourceInterface == "")
        {
            resourceInterface = (*iter).get<string>();
            break;
        }
    }

    if (discoverable)
        resourceProperty |= OC_DISCOVERABLE;

    if (observable)
        resourceProperty |= OC_OBSERVABLE;

    if (isSecure)
        resourceProperty |= OC_SECURE;

    DEBUG_MSG("discoverable=%d, observable=%d, isSecure=%d\n", discoverable, observable, isSecure);

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

    DEBUG_MSG("OCPlatform::registerResource: result=%d\n",result);

    if (OC_STACK_OK != result)
    {
        std::cerr << "OCPlatform::registerResource was unsuccessful\n";
        postError(async_call_id);
        return;
    }

    postRegisterResource(async_call_id, resHandle, param);
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

    double async_call_id = value.get("asyncCallId").get<double>();

    std::shared_ptr<OCResourceResponse> resourceResponse =
                   {std::make_shared<OCResourceResponse>()};

    resourceResponse->setErrorCode(200);
    //TODO resourceResponse->setResourceRepresentation(lightPtr->get(), DEFAULT_INTERFACE);
/*
    result = OCPlatform::notifyListOfObservers(lightPtr->getHandle(),
                                               lightPtr->m_interestedObservers,
                                               resourceResponse);
*/
}


void IotivityInstance::handleSendResponse(const picojson::value& value) {

    double async_call_id = value.get("asyncCallId").get<double>();
    picojson::value resource = value.get("resource");


    std::shared_ptr<OCResourceRequest> request; // TODO init

    std::string requestType = request->getRequestType();
    int requestFlag = request->getRequestHandlerFlag();
    OCRepresentation oCRepresentation = request->getResourceRepresentation();

    auto pResponse = std::make_shared<OC::OCResourceResponse>();


    if (requestFlag & RequestHandlerFlag::RequestFlag)
    {
        if (requestType == "retrieve")
        {
            pResponse->setRequestHandle(request->getRequestHandle());
            pResponse->setResourceHandle(request->getResourceHandle());
            pResponse->setResourceRepresentation(oCRepresentation);
            pResponse->setErrorCode(200);
            pResponse->setResponseResult(OC_EH_OK);

        }
        else if (requestType == "update")
        {
            pResponse->setRequestHandle(request->getRequestHandle());
            pResponse->setResourceHandle(request->getResourceHandle());
            pResponse->setResourceRepresentation(oCRepresentation);
            pResponse->setErrorCode(200);
            pResponse->setResponseResult(OC_EH_OK);
        }
        else if (requestType == "create") // POST (first time)
        {
            pResponse->setRequestHandle(request->getRequestHandle());
            pResponse->setResourceHandle(request->getResourceHandle());
            //OCRepresentation rep = pRequest->getResourceRepresentation();
            //OCRepresentation rep_post = post(rep);
            pResponse->setResourceRepresentation(oCRepresentation);
            pResponse->setErrorCode(200);
            pResponse->setResponseResult(OC_EH_OK);
        }

        OCStackResult result = OCPlatform::sendResponse(pResponse);
        if (OC_STACK_OK != result)
        {
            std::cerr << "OCPlatform::sendResponse was unsuccessful\n";
            postError(async_call_id);
            return;
        }
    }


    if(requestFlag & RequestHandlerFlag::ObserverFlag)
    {
  

    }


    //pResponse->setRequestHandle(value.get("requestId").get<double>());
    // TODO pResponse->setResourceHandle(request->getResourceHandle());
    //pResponse->setHeaderOptions(serverHeaderOptions);
    // TODO pResponse->setResourceRepresentation(get(), "");

    postResult("sendResponseCompleted", async_call_id);
}

void IotivityInstance::handleSendError(const picojson::value& value) {

    double async_call_id = value.get("asyncCallId").get<double>();

    auto pResponse = std::make_shared<OC::OCResourceResponse>();

    //pResponse->setRequestHandle(value.get("requestId").get<double>());
    // TODOpResponse->setResourceHandle(request->getResourceHandle());
    //pResponse->setHeaderOptions(serverHeaderOptions);

    pResponse->setErrorCode(value.get("error").get<double>());
    pResponse->setResponseResult(OC_EH_OK);

    OCStackResult result = OCPlatform::sendResponse(pResponse);
    if (OC_STACK_OK != result)
    {
        std::cerr << "OCPlatform::sendResponse was unsuccessful\n";
        postError(async_call_id);
        return;
    }

    postResult("sendResponseCompleted", async_call_id);
}





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


#include <functional>
#include <pthread.h>
#include <mutex>
#include <condition_variable>

char *pDebugEnv = NULL;
const int SUCCESS_RESPONSE = 0;


std::map<int, OCRepresentation> ResourcesMap;


#define INFO_MSG(msg, ...) { printf(msg, ##__VA_ARGS__);}
#define DEBUG_MSG(msg, ...) { if (pDebugEnv) printf(msg, ##__VA_ARGS__);}


static void PrintfOcResource(const OCResource & oCResource) {

    DEBUG_MSG("Res[sId] = %s\n", oCResource.sid().c_str());
    DEBUG_MSG("Res[Uri] = %s\n", oCResource.uri().c_str());
    DEBUG_MSG("Res[Host] = %s\n", oCResource.host().c_str());

    DEBUG_MSG("Res[Resource types] \n");
    for(auto &resourceTypes : oCResource.getResourceTypes())
    {
        DEBUG_MSG("\t\t%s\n", resourceTypes.c_str());
    }

    DEBUG_MSG("Res[Resource interfaces] \n");
    for(auto &resourceInterfaces : oCResource.getResourceInterfaces())
    {
        DEBUG_MSG("\t\t%s\n", resourceInterfaces.c_str());
    }
}

static void PrintfOcRepresentation(const OCRepresentation & oCRepresentation) {

    for (auto& cur: oCRepresentation)
    {
        std::string attrname = cur.attrname();
        if (AttributeType::String == cur.type())
        {
            std::string curStr = cur.getValue<string>();
            DEBUG_MSG("Rep[String]: key=%s, value=%s\n", attrname.c_str(), curStr.c_str());
        }
        else if (AttributeType::Integer == cur.type())
        {
            DEBUG_MSG("Rep[String]: key=%s, value=%d\n", attrname.c_str(), cur.getValue<int>());
        }
        else if (AttributeType::Double == cur.type())
        {
            DEBUG_MSG("Rep[String]: key=%s, value=%f\n", attrname.c_str(), cur.getValue<double>());
        }
        else if (AttributeType::Boolean == cur.type())
        {
            DEBUG_MSG("Rep[String]: key=%s, value=%d\n", attrname.c_str(), cur.getValue<bool>());
        }
    }
}

// Translate OCRepresentation to picojson
static void TranslateOCRepresentationToPicojson(const OCRepresentation & oCRepresentation, picojson::object & objectRes) {

    for (auto& cur: oCRepresentation)
    {
        std::string attrname = cur.attrname();
        if (AttributeType::String == cur.type())
        {
            std::string curStr = cur.getValue<string>();
            objectRes[attrname] = picojson::value(curStr);
        }
        else if (AttributeType::Integer == cur.type())
        {           
            int intValue = cur.getValue<int>();
            objectRes[attrname] = picojson::value((double)intValue);
        }
        else if (AttributeType::Double == cur.type())
        {
            int doubleValue = cur.getValue<double>();
            objectRes[attrname] = picojson::value((double)doubleValue);
        }
        else if (AttributeType::Boolean == cur.type())
        {
            bool boolValue = cur.getValue<bool>();
            objectRes[attrname] = picojson::value((bool)boolValue);
        }
    }
}

// TODO Add std::map ??
OCResourceHandle resHandle;

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

    DEBUG_MSG("handleConfigure: v=%s\n",value.serialize().c_str());

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

void IotivityInstance::foundResourceCallback(std::shared_ptr<OCResource> resource) {

    DEBUG_MSG("foundResourceCallback:\n");

    picojson::value::object object;
    object["cmd"] = picojson::value("foundResourceCallback");

    //object["OicResourceInit"] = param;

    DEBUG_MSG("DISCOVERED Resource:\n");
    PrintfOcResource((const OCResource &)*resource);

    object["resourceId"] = picojson::value((double)((int)resource.get()));
    object["url"] = picojson::value(resource->uri());

    //resource->connectivityType()
    object["connectionMode"] = picojson::value("default");
    object["discoverable"] = picojson::value(resource->isObservable()); // TODO
    object["observable"] = picojson::value(resource->isObservable());

    picojson::value value(object);
    PostMessage(value.serialize().c_str());
}


void IotivityInstance::handleFindResources(const picojson::value& value) {

    DEBUG_MSG("handleFindResources: v=%s\n",value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    picojson::value param = value.get("OicDiscoveryOptions");

    // all properties are null by default, meaning “find all”
    // if resourceId is specified in full form, a direct retrieve is made
    // if resourceType is specified, a retrieve on /oic/res is made
    // if resourceId is null, and deviceId not, then only resources from that device are returned
    std::string deviceId = param.get("deviceId").to_str();
    std::string resourceId = param.get("resourceId").to_str();
    std::string resourceType = param.get("resourceType").to_str();
 
/*
    PlatformConfig cfg
    {
        ServiceType::InProc,
        ModeType::Client,
        "0.0.0.0", // By setting to "0.0.0.0", it binds to all available interfaces
        0,         // Uses randomly available port
        QualityOfService::LowQos
    };

    OCPlatform::Configure(cfg);
*/
    std:string hostUri = OC_MULTICAST_DISCOVERY_URI;
    string requestUri = hostUri + "?rt=" + resourceType;

    FindCallback resourceHandler = std::bind(&IotivityInstance::foundResourceCallback, 
                                             this, 
                                             std::placeholders::_1);
    OCStackResult result = OCPlatform::findResource("", 
                                                    requestUri,
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

    DEBUG_MSG("foundDeviceCallback:\n");

}

void IotivityInstance::handleFindDevices(const picojson::value& value) {

    DEBUG_MSG("handleFindDevices: v=%s\n",value.serialize().c_str());

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

    DEBUG_MSG("onPost: onPut=%d\n", eCode);

    if (eCode == SUCCESS_RESPONSE)
    {

    }
    else
    {

    }
}

void IotivityInstance::onGet(const HeaderOptions& headerOptions, const OCRepresentation& rep, const int eCode) {

    DEBUG_MSG("onPost: onGet=%d\n", eCode);

    if (eCode == SUCCESS_RESPONSE)
    {
        //postResult("retrieveResourceCompleted", async_call_id);
    }
    else
    {

    }
}

void IotivityInstance::onPost(const HeaderOptions& headerOptions, const OCRepresentation& rep, const int eCode) {

    DEBUG_MSG("onPost: eCode=%d\n", eCode);

    if (eCode == SUCCESS_RESPONSE)
    {
    }
    else
    {

    }
}

void IotivityInstance::onObserve(const HeaderOptions headerOptions, const OCRepresentation& rep,
               const int& eCode, const int& sequenceNumber) {
 
    DEBUG_MSG("onObserve: sequenceNumber=%d, eCode=%d\n", sequenceNumber, eCode);

    picojson::value::object object;
    object["cmd"] = picojson::value("onObserve");
    object["requestId"] = picojson::value((double)0);
    object["source"] = picojson::value((double)0);
    object["target"] = picojson::value((double)0);
    //object["requestId"] = picojson::value((double)((int)request->getRequestHandle()));
    //object["source"] = picojson::value((double)((int)request->getRequestHandle())); // Client UUID
    //object["target"] = picojson::value((double)((int)request->getResourceHandle()));
    object["type"] = picojson::value("observe");

    if (eCode == OC_STACK_OK)
    {

    }
    else
    {

    }

    picojson::value value(object);
    PostMessage(value.serialize().c_str());
}   

void IotivityInstance::onDelete(const HeaderOptions& headerOptions, const int eCode) {

    DEBUG_MSG("onDelete: eCode=%d\n", eCode);

    if (eCode == SUCCESS_RESPONSE)
    {
    }
    else
    {

    }
}

void IotivityInstance::handleCreateResource(const picojson::value& value) {

    // Post + particular data
    DEBUG_MSG("handleCreateResource: v=%s\n",value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    picojson::value param = value.get("OicResourceInit");
    int resHandleInt = (int)param.get("deviceId").get<double>();
    OCResource *resource = (OCResource *)(resHandleInt);
    if (resource)
    {
        OCRepresentation representation;

        PostCallback attributeHandler = std::bind(&IotivityInstance::onPost, 
                                                  this, 
                                                  std::placeholders::_1,
                                                  std::placeholders::_2,
                                                  std::placeholders::_3);
        OCStackResult result = resource->post(representation,
                                              QueryParamsMap(), 
                                              attributeHandler);
        if (OC_STACK_OK != result)
        {
            std::cerr << "post/create was unsuccessful\n";
            postError(async_call_id);
            return;
        }
    }
}

void IotivityInstance::handleRetrieveResource(const picojson::value& value) {
    DEBUG_MSG("handleRetrieveResource: v=%s\n",value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    int resHandleInt = (int)value.get("resourceId").get<double>();
    OCResource *resource = (OCResource *)(resHandleInt);
    if (resource)
    {
        GetCallback attributeHandler = std::bind(&IotivityInstance::onGet, 
                                                 this, 
                                                 std::placeholders::_1,
                                                 std::placeholders::_2,
                                                 std::placeholders::_3);
        OCStackResult result = resource->get(QueryParamsMap(), 
                                             attributeHandler);
        if (OC_STACK_OK != result)
        {
            std::cerr << "get was unsuccessful\n";
            postError(async_call_id);
            return;
        }
    }
}

void IotivityInstance::handleUpdateResource(const picojson::value& value) {

    DEBUG_MSG("handleUpdateResource: v=%s\n",value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    picojson::value param = value.get("OicResource");
    int resHandleInt = (int)param.get("id").get<double>();
    OCResource *resource = (OCResource *)(resHandleInt);
    if (resource)
    {
        OCRepresentation representation;
        //PostCallback ?? TODO
        PutCallback attributeHandler = std::bind(&IotivityInstance::onPut, 
                                                 this, 
                                                 std::placeholders::_1,
                                                 std::placeholders::_2,
                                                 std::placeholders::_3);
        OCStackResult result = resource->put(representation,
                                             QueryParamsMap(), 
                                             attributeHandler);
        if (OC_STACK_OK != result)
        {
            std::cerr << "put was unsuccessful\n";
            postError(async_call_id);
            return;
        }
    }

}

void IotivityInstance::handleDeleteResource(const picojson::value& value) {
    DEBUG_MSG("handleDeleteResource: v=%s\n",value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    int resHandleInt = (int)value.get("resourceId").get<double>();
    OCResource *resource = (OCResource *)(resHandleInt);
    if (resource)
    {
        DeleteCallback deleteHandler = std::bind(&IotivityInstance::onDelete, 
                                                 this, 
                                                 std::placeholders::_1,
                                                 std::placeholders::_2);
        OCStackResult result = resource->deleteResource(deleteHandler);
        if (OC_STACK_OK != result)
        {
            std::cerr << "delete was unsuccessful\n";
            postError(async_call_id);
            return;
        }
    }
}

void IotivityInstance::handleStartObserving(const picojson::value& value) {

    DEBUG_MSG("handleStartObserving: v=%s\n",value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    int resHandleInt = (int)value.get("resourceId").get<double>();
    OCResource *resource = (OCResource *)(resHandleInt);

    DEBUG_MSG("handleStartObserving:resource=%d\n", resource);
    //DEBUG_MSG("handleStartObserving:*resource=%d\n", *resource);
    if (resource)
    {
        ObserveCallback observeHandler = std::bind(&IotivityInstance::onObserve, 
                                                   this, 
                                                   std::placeholders::_1,
                                                   std::placeholders::_2,
                                                   std::placeholders::_3,
                                                   std::placeholders::_4);
DEBUG_MSG("handleStartObserving:3\n");

        std::shared_ptr<OCResource> curResource(resource);
        OCStackResult result = curResource->observe(ObserveType::ObserveAll, 
                                                    QueryParamsMap(), 
                                                    observeHandler);
        if (OC_STACK_OK != result)
        {
            std::cerr << "observe was unsuccessful\n";
            postError(async_call_id);
            return;
        }
    }

DEBUG_MSG("handleStartObserving:4\n");
    // TODO: retrieve + observe flag// retrieve + observe flag
    postResult("startObservingCompleted", async_call_id);
}

void IotivityInstance::handleCancelObserving(const picojson::value& value) {
    DEBUG_MSG("handleCancelObserving: v=%s\n",value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    int resHandleInt = (int)value.get("resourceId").get<double>();
    OCResource *resource = (OCResource *)(resHandleInt);
    if (resource)
    {
        OCStackResult result = resource->cancelObserve();
        if (OC_STACK_OK != result)
        {
            std::cerr << "cancelObserve was unsuccessful\n";
            postError(async_call_id);
            return;
        }
    }

    postResult("cancelObservingCompleted", async_call_id);
}

void IotivityInstance::postEntityHandler(std::shared_ptr<OCResourceRequest> request) {

    picojson::value::object object;
    object["cmd"] = picojson::value("entityHandler");

    std::string requestType = request->getRequestType();
    int requestFlag = request->getRequestHandlerFlag();
    QueryParamsMap queries = request->getQueryParameters();

    if (!queries.empty())
    {
        DEBUG_MSG("Query processing\n");

        for (auto it : queries)
        {
            DEBUG_MSG("Queries: key=%s, value=%s\n",it.first.c_str(), it.second.c_str());
        }
    }

    DEBUG_MSG("postEntityHandler: requestType=%s\n",requestType.c_str());
    DEBUG_MSG("postEntityHandler: requestHandle=0x%x\n", (int)request->getRequestHandle());

    object["requestId"] = picojson::value((double)((int)request->getRequestHandle()));
    object["source"] = picojson::value((double)((int)request->getRequestHandle())); // Client UUID
    object["target"] = picojson::value((double)((int)request->getResourceHandle()));

    if(requestFlag & RequestHandlerFlag::RequestFlag)
    {
        DEBUG_MSG("postEntityHandler:RequestFlag\n");

        if (requestType == "GET")
        {
            object["type"] = picojson::value("retrieve");
        }
        else if (requestType == "PUT")
        {
            object["type"] = picojson::value("update");

            OCRepresentation oCRepresentation = request->getResourceRepresentation();
            PrintfOcRepresentation(oCRepresentation);

            // Translate OCRepresentation to picojson
            picojson::object objectRes;
            TranslateOCRepresentationToPicojson(oCRepresentation, objectRes);
            object["res"] = picojson::value(objectRes);

            // Check for matched properties
            //updatedPropertyNames;

        }
        else if (requestType == "POST")
        {
            object["type"] = picojson::value("observe");

        }
        else if (requestType == "DELETE")
        {
            object["type"] = picojson::value("delete");
        }
        else if (requestType == "CREATE")
        {
            object["type"] = picojson::value("create");

            OCRepresentation oCRepresentation = request->getResourceRepresentation();
            // Translate OCRepresentation to picojson
            picojson::object objectRes;
            TranslateOCRepresentationToPicojson(oCRepresentation, objectRes);
            object["res"] = picojson::value(objectRes);
        }
    }

    if (requestFlag & RequestHandlerFlag::ObserverFlag)
    {
        DEBUG_MSG("postEntityHandler:ObserverFlag\n");

        object["type"] = picojson::value("observe");

        ObservationInfo observationInfo = request->getObservationInfo();
        if(ObserveAction::ObserveRegister == observationInfo.action)
        {
            DEBUG_MSG("postEntityHandler:ObserveRegister\n");
            //m_interestedObservers.push_back(observationInfo.obsId);
        }
        else if(ObserveAction::ObserveUnregister == observationInfo.action)
        {
            DEBUG_MSG("postEntityHandler:ObserveUnregister\n");
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

    //object["queryOptions"] = picojson::value(queries);

    HeaderOptions headerOptions = request->getHeaderOptions();
    //object["headerOptions"] = picojson::value(headerOptions);


    picojson::value value(object);
    PostMessage(value.serialize().c_str());
}

OCEntityHandlerResult IotivityInstance::entityHandlerCallback(std::shared_ptr<OCResourceRequest> request) {

    OCEntityHandlerResult ehResult = OC_EH_ERROR;
    if(request)
    {
        ehResult = OC_EH_OK;
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

  DEBUG_MSG("postResult: c=%s, id=%f\n", completed_operation, async_operation_id);

  picojson::value::object object;
  object["cmd"] = picojson::value(completed_operation);
  object["asyncCallId"] = picojson::value(async_operation_id);

  picojson::value value(object);
  PostMessage(value.serialize().c_str());
}

void IotivityInstance::postError(double async_operation_id) {

  DEBUG_MSG("postError: id=%d\n",async_operation_id);

  picojson::value::object object;
  object["cmd"] = picojson::value("asyncCallError");
  object["asyncCallId"] = picojson::value(async_operation_id);

  picojson::value value(object);
  PostMessage(value.serialize().c_str());
}

void IotivityInstance::postRegisterResource(double async_operation_id, OCResourceHandle resHandle, const picojson::value& param) {

  DEBUG_MSG("postRegisterResource: v=%s\n",param.serialize().c_str());

  picojson::value::object object;
  object["cmd"] = picojson::value("registerResourceCompleted");
  object["asyncCallId"] = picojson::value(async_operation_id);
  object["OicResourceInit"] = param;
  object["resourceId"] = picojson::value((double)((int)resHandle));

  picojson::value value(object);
  PostMessage(value.serialize().c_str());
}

void IotivityInstance::handleRegisterResource(const picojson::value& value) {

    DEBUG_MSG("handleRegisterResource: v=%s\n",value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    picojson::value param = value.get("OicResourceInit");

    std::string deviceId = param.get("deviceId").to_str();
    std::string connectionMode = param.get("connectionMode").to_str();
    picojson::array resourceTypes = param.get("resourceTypes").get<picojson::array>();
    picojson::array interfaces = param.get("interfaces").get<picojson::array>();
    bool discoverable = param.get("discoverable").get<bool>();
    bool observable = param.get("observable").get<bool>();
    bool isSecure = false;

    std::string resourceURI = param.get("url").to_str();
    std::vector<std::string> resourceTypeNameArray;
    std::string resourceTypeName = "";

    std::vector<std::string> resourceInterfaceArray;
    std::string resourceInterface = "";
    uint8_t resourceProperty = 0;

    picojson::value properties = param.get("properties");


    for (picojson::array::iterator iter = resourceTypes.begin(); iter != resourceTypes.end(); ++iter) {
        DEBUG_MSG("array resourceTypes value=%s\n", (*iter).get<string>().c_str());
        if (resourceTypeName == "")
        {
            resourceTypeName = (*iter).get<string>();          
        }

        resourceTypeNameArray.push_back((*iter).get<string>());
    }

    for (picojson::array::iterator iter = interfaces.begin(); iter != interfaces.end(); ++iter) {
        DEBUG_MSG("array interfaces value=%s\n", (*iter).get<string>().c_str());
        if (resourceInterface == "")
        {
            resourceInterface = (*iter).get<string>();
        }
        resourceInterfaceArray.push_back((*iter).get<string>());
    }

    if (resourceInterface == "")
        resourceInterface = DEFAULT_INTERFACE;

    if (discoverable)
        resourceProperty |= OC_DISCOVERABLE;

    if (observable)
        resourceProperty |= OC_OBSERVABLE;

    if (isSecure)
        resourceProperty |= OC_SECURE;

    DEBUG_MSG("discoverable=%d, observable=%d, isSecure=%d\n", discoverable, observable, isSecure);
    DEBUG_MSG("SVR: uri=%s, type=%s, itf=%s, prop=%d\n", 
              resourceURI.c_str(), 
              resourceTypeName.c_str(), 
              resourceInterface.c_str(), 
              resourceProperty);
/*
    PlatformConfig cfg
    {
        ServiceType::InProc,
        ModeType::Server,
        "0.0.0.0", // By setting to "0.0.0.0", it binds to all available interfaces
        0,         // Uses randomly available port
        QualityOfService::LowQos
    };

    OCPlatform::Configure(cfg);
*/

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
        std::cerr << "registerResource was unsuccessful\n";
        postError(async_call_id);
        return;
    }

    if (resourceTypeNameArray.size() >= 2)
    {
        for (int i = 1; i < resourceTypeNameArray.size(); i++)
        {
            resourceTypeName = resourceTypeNameArray[i];

            DEBUG_MSG("bindTypeToResource=%s\n", resourceTypeName.c_str());

            result = OCPlatform::bindTypeToResource(resHandle, resourceTypeName);
            if (OC_STACK_OK != result)
            {
                std::cerr << "bindTypeToResource TypeName to Resource was unsuccessful\n";
                postError(async_call_id);
               return;
            }
        }
    }

    if (resourceInterfaceArray.size() >= 2)
    {
        for (int i = 1; i < resourceInterfaceArray.size(); i++)
        {
            resourceInterface = resourceInterfaceArray[i];

            DEBUG_MSG("bindInterfaceToResource=%s\n", resourceInterface.c_str());

            result = OCPlatform::bindInterfaceToResource(resHandle, resourceInterface);
            if (OC_STACK_OK != result)
            {
                std::cerr << "Binding InterfaceName to Resource was unsuccessful\n";
                postError(async_call_id);
                return;
            }
        }
    }
/*
    // TODO integrate representation properties = param.get("properties");
    OCRepresentation *oCRepresentation = new OCRepresentation();
    if (oCRepresentation == NULL) {
        std::cerr << "Binding InterfaceName to Resource was unsuccessful\n";
        postError(async_call_id);
        return;
    }
    ResourcesMap[(int)resHandle] = oCRepresentation;
*/


    postRegisterResource(async_call_id, resHandle, param);
}

void IotivityInstance::handleUnregisterResource(const picojson::value& value) {

    DEBUG_MSG("handleUnregisterResource: v=%s\n",value.serialize().c_str());

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

    DEBUG_MSG("handleEnablePresence: v=%s\n",value.serialize().c_str());

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

    DEBUG_MSG("handleDisablePresence: v=%s\n",value.serialize().c_str());

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

    DEBUG_MSG("handleNotify: v=%s\n",value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    int resourceId = (int)value.get("resourceId").get<double>();
    std::string method = value.get("method").to_str();
    picojson::value updatedPropertyNames = value.get("updatedPropertyNames");
   

    OCStackResult result = OCPlatform::notifyAllObservers((OCResourceHandle)resourceId);
    if (OC_STACK_OK != result)
    {
        std::cerr << "OCPlatform::notifyAllObservers was unsuccessful\n";
        postError(async_call_id);
        return;
    }

    postResult("notifyCompleted", async_call_id);
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
        postError(async_call_id);
        return;
    }

    postResult("sendResponseCompleted", async_call_id);
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
        postError(async_call_id);
        return;
    }

    DEBUG_MSG("handleSendError:4\n");
    postResult("sendResponseCompleted", async_call_id);
}





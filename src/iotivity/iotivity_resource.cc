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
#include "iotivity/iotivity_resource.h"
#include "common/extension.h"


IotivityResourceInit::IotivityResourceInit() {

    m_url = "";
    m_deviceId = "";
    m_connectionMode = "";
    m_discoverable = false;
    m_observable = false;
    m_isSecure = false;
}

IotivityResourceInit::IotivityResourceInit(const picojson::value& value) {

    m_url = "";
    m_deviceId = "";
    m_connectionMode = "";
    m_discoverable = false;
    m_observable = false;
    m_isSecure = false;

    Deserialize(value);
}

IotivityResourceInit::~IotivityResourceInit() {

}


void IotivityResourceInit::Deserialize(const picojson::value& value) {

    m_url = value.get("url").to_str();
    m_deviceId = value.get("deviceId").to_str();
    m_connectionMode = value.get("connectionMode").to_str();
    m_discoverable = value.get("discoverable").get<bool>();
    m_observable = value.get("observable").get<bool>();
    m_isSecure = false;

    m_resourceTypeName = "";
    m_resourceInterface = "";
    m_resourceProperty = 0;


    picojson::array resourceTypes = value.get("resourceTypes").get<picojson::array>();
    for (picojson::array::iterator iter = resourceTypes.begin(); iter != resourceTypes.end(); ++iter) {
        DEBUG_MSG("array resourceTypes value=%s\n", (*iter).get<string>().c_str());
        if (m_resourceTypeName == "")
        {
            m_resourceTypeName = (*iter).get<string>();          
        }

        m_resourceTypeNameArray.push_back((*iter).get<string>());
    }

    picojson::array interfaces = value.get("interfaces").get<picojson::array>();
    for (picojson::array::iterator iter = interfaces.begin(); iter != interfaces.end(); ++iter) {
        DEBUG_MSG("array interfaces value=%s\n", (*iter).get<string>().c_str());
        if (m_resourceInterface == "")
        {
            m_resourceInterface = (*iter).get<string>();
        }
        m_resourceInterfaceArray.push_back((*iter).get<string>());
    }

    if (m_resourceInterface == "")
        m_resourceInterface = DEFAULT_INTERFACE;

    if (m_discoverable)
        m_resourceProperty |= OC_DISCOVERABLE;

    if (m_observable)
        m_resourceProperty |= OC_OBSERVABLE;

    if (m_isSecure)
        m_resourceProperty |= OC_SECURE;

    DEBUG_MSG("discoverable=%d, observable=%d, isSecure=%d\n", m_discoverable, m_observable, m_isSecure);
    DEBUG_MSG("SVR: uri=%s, type=%s, itf=%s, prop=%d\n", 
              m_url.c_str(), 
              m_resourceTypeName.c_str(), 
              m_resourceInterface.c_str(), 
              m_resourceProperty);


    picojson::value properties = value.get("properties");
/*
    // TODO integrate representation
    for (picojson::value::iterator iter = properties.begin(); iter != properties.end(); ++iter) {
        std::string objectKey = (*iter).get<string>();
        std::string objectValue = (*iter)[objectKey];
        DEBUG_MSG("properties key=%s, value=%s\n", objectKey.c_str(), objectValue..c_str());

        m_resourceRep[objectKey] = objectValue;
    }
*/

}


void IotivityResourceInit::Serialize(picojson::object& object) {

    object["url"] = picojson::value(m_url);
    object["deviceId"] = picojson::value(m_deviceId);
    object["connectionMode"] = picojson::value(m_connectionMode);
    object["discoverable"] = picojson::value(m_discoverable);
    object["observable"] = picojson::value(m_observable);


    picojson::array resourceTypes;
    for (int i = 0; i < m_resourceTypeNameArray.size(); i++)
    {
        std::string resourceTypeName = m_resourceTypeNameArray[i];
        resourceTypes.push_back(picojson::value(resourceTypeName));
    }
    object["resourceTypes"] = picojson::value(resourceTypes);


    picojson::array interfaces;
    for (int i = 0; i < m_resourceInterfaceArray.size(); i++)
    {
        std::string resourceInterfaceName = m_resourceInterfaceArray[i];
        interfaces.push_back(picojson::value(resourceInterfaceName));

    }
    object["interfaces"] = picojson::value(interfaces);

    
    picojson::object properties;
    TranslateOCRepresentationToPicojson(m_resourceRep, properties);
    object["properties"] = picojson::value(properties);
}


IotivityResourceServer::IotivityResourceServer(IotivityDevice *device, IotivityResourceInit *oicResource) : m_device(device) {

    m_oicResourceInit = oicResource;

    m_resourceHandle = 0;

}

IotivityResourceServer::~IotivityResourceServer() {
    delete m_oicResourceInit;

   if (m_resourceHandle != 0)
   {

   }
}

OCEntityHandlerResult IotivityResourceServer::entityHandlerCallback(std::shared_ptr<OCResourceRequest> request) {

    OCEntityHandlerResult ehResult = OC_EH_ERROR;
    if(request)
    {
        ehResult = OC_EH_OK;


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
            m_interestedObservers.push_back(observationInfo.obsId);
        }
        else if(ObserveAction::ObserveUnregister == observationInfo.action)
        {
            DEBUG_MSG("postEntityHandler:ObserveUnregister\n");
            m_interestedObservers.erase(std::remove(m_interestedObservers.begin(),
                                                    m_interestedObservers.end(),
                                                    observationInfo.obsId),
                                                    m_interestedObservers.end());
        }
    }
    else if  (requestFlag & RequestHandlerFlag::RequestFlag)
    {

    }

    // TODO
    //object["queryOptions"] = picojson::value(queries);

    HeaderOptions headerOptions = request->getHeaderOptions();
    //object["headerOptions"] = picojson::value(headerOptions);


    picojson::value value(object);
    m_device->getInstance()->PostMessage(value.serialize().c_str());


        
    }
    else
    {
        std::cerr << "entityHandlerCallback: Request invalid" << std::endl;
    }

    return ehResult;
}

void IotivityResourceServer::Serialize(picojson::object& object) {

    object["id"] = picojson::value((double)getResourceHandleToInt());

    picojson::object properties;
    m_oicResourceInit->Serialize(properties);
    object["properties"] = picojson::value(properties);
}



OCStackResult IotivityResourceServer::registerResource() {

    OCStackResult result = OC_STACK_ERROR;

    EntityHandler resourceCallback = std::bind(&IotivityResourceServer::entityHandlerCallback, 
                                               this, 
                                               std::placeholders::_1);

    result = OCPlatform::registerResource(m_resourceHandle, 
                                          m_oicResourceInit->m_url, 
                                          m_oicResourceInit->m_resourceTypeName,
                                          m_oicResourceInit->m_resourceInterface, 
                                          resourceCallback, 
                                          m_oicResourceInit->m_resourceProperty);
    if (OC_STACK_OK != result)
    {
        std::cerr << "registerResource was unsuccessful\n";
        return result;
    }

    if (m_oicResourceInit->m_resourceTypeNameArray.size() >= 2)
    {
        for (int i = 1; i < m_oicResourceInit->m_resourceTypeNameArray.size(); i++)
        {
            std::string resourceTypeName = m_oicResourceInit->m_resourceTypeNameArray[i];

            DEBUG_MSG("bindTypeToResource=%s\n", resourceTypeName.c_str());

            result = OCPlatform::bindTypeToResource(m_resourceHandle, resourceTypeName);
            if (OC_STACK_OK != result)
            {
                std::cerr << "bindTypeToResource TypeName to Resource was unsuccessful\n";
                return result;
            }
        }
    }

    if (m_oicResourceInit->m_resourceInterfaceArray.size() >= 2)
    {
        for (int i = 1; i < m_oicResourceInit->m_resourceInterfaceArray.size(); i++)
        {
            std::string resourceInterface = m_oicResourceInit->m_resourceInterfaceArray[i];

            DEBUG_MSG("bindInterfaceToResource=%s\n", resourceInterface.c_str());

            result = OCPlatform::bindInterfaceToResource(m_resourceHandle, resourceInterface);
            if (OC_STACK_OK != result)
            {
                std::cerr << "Binding InterfaceName to Resource was unsuccessful\n";
                return result;
            }
        }
    }

    return result;
}

int IotivityResourceServer::getResourceHandleToInt() {

    return ((int)m_resourceHandle);
}





IotivityResourceClient::IotivityResourceClient(IotivityDevice *device) : m_device(device) {

    m_ocResourcePtr = NULL;
    m_oicResourceInit = new IotivityResourceInit();
}

IotivityResourceClient::~IotivityResourceClient() {
}

void IotivityResourceClient::setSharedPtr(std::shared_ptr<OCResource> sharePtr) {

    m_ocResourcePtr = sharePtr;

    m_id = (int)sharePtr.get();

    m_oicResourceInit->m_url = sharePtr->uri();
    m_oicResourceInit->m_deviceId = std::to_string(m_id);

    m_oicResourceInit->m_connectionMode = "default"; // TODO : m_discoverable missing info from shared_ptr
    m_oicResourceInit->m_discoverable = sharePtr->isObservable();
    m_oicResourceInit->m_observable = sharePtr->isObservable();
    m_oicResourceInit->m_isSecure = false;
    
    m_sid =  sharePtr->sid();
    m_host =  sharePtr->host();

    for(auto &resourceTypes : sharePtr->getResourceTypes())
    {
        m_oicResourceInit->m_resourceTypeNameArray.push_back(resourceTypes.c_str());
    }

    for(auto &resourceInterfaces : sharePtr->getResourceInterfaces())
    {
        m_oicResourceInit->m_resourceInterfaceArray.push_back(resourceInterfaces.c_str());
    }
}

int IotivityResourceClient::getResourceHandleToInt() {

    return ((int)m_id);
}


void IotivityResourceClient::Serialize(picojson::object& object) {

    object["id"] = picojson::value((double)getResourceHandleToInt());

    picojson::object properties;
    m_oicResourceInit->Serialize(properties);
    object["properties"] = picojson::value(properties);
}



void IotivityResourceClient::onPut(const HeaderOptions& headerOptions, const OCRepresentation& rep, const int eCode) {

    DEBUG_MSG("onPut: eCode=%d\n", eCode);

    picojson::value::object object;
    object["cmd"] = picojson::value("updateResourceCompleted");
    object["eCode"] = picojson::value((double)eCode);

    if (eCode == SUCCESS_RESPONSE)
    {
        PrintfOcRepresentation(rep);
    }
    else
    {
        std::cerr << "onPut was unsuccessful\n";
    }

    picojson::value value(object);
    m_device->getInstance()->PostMessage(value.serialize().c_str());
}

void IotivityResourceClient::onGet(const HeaderOptions& headerOptions, const OCRepresentation& rep, const int eCode) {

    DEBUG_MSG("onGet: eCode=%d\n", eCode);

    picojson::value::object object;
    object["cmd"] = picojson::value("retrieveResourceCompleted");
    object["requestId"] = picojson::value((double)0);
    object["source"] = picojson::value((double)0);
    object["target"] = picojson::value((double)0);
    //object["requestId"] = picojson::value((double)((int)request->getRequestHandle()));
    //object["source"] = picojson::value((double)((int)request->getRequestHandle())); // Client UUID
    //object["target"] = picojson::value((double)((int)request->getResourceHandle()));
    object["eCode"] = picojson::value((double)eCode);

    if (eCode == SUCCESS_RESPONSE)
    {
        PrintfOcRepresentation(rep);

        // Translate OCRepresentation to picojson
        picojson::object objectRes;
        TranslateOCRepresentationToPicojson(rep, objectRes);
        object["res"] = picojson::value(objectRes);
    }
    else
    {
        std::cerr << "onGet was unsuccessful\n";
    }

    picojson::value value(object);
    m_device->getInstance()->PostMessage(value.serialize().c_str());
}

void IotivityResourceClient::onPost(const HeaderOptions& headerOptions, const OCRepresentation& rep, const int eCode) {

    DEBUG_MSG("onPost: eCode=%d\n", eCode);

    if (eCode == SUCCESS_RESPONSE)
    {
        PrintfOcRepresentation(rep);
    }
    else
    {

    }
}

void IotivityResourceClient::onObserve(const HeaderOptions headerOptions, const OCRepresentation& rep,
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
        PrintfOcRepresentation(rep);
    }
    else
    {

    }

    picojson::value value(object);
    m_device->getInstance()->PostMessage(value.serialize().c_str());
}   

void IotivityResourceClient::onDelete(const HeaderOptions& headerOptions, const int eCode) {

    DEBUG_MSG("onDelete: eCode=%d\n", eCode);

    picojson::value::object object;
    object["cmd"] = picojson::value("deleteResourceCompleted");
    object["eCode"] = picojson::value((double)eCode);

    if (eCode == SUCCESS_RESPONSE)
    {

    }
    else
    {
        std::cerr << "onDelete was unsuccessful\n";
    }

    picojson::value value(object);
    m_device->getInstance()->PostMessage(value.serialize().c_str());
}


OCStackResult IotivityResourceClient::createResource(IotivityResourceInit *oicResourceInit) {

    OCStackResult result = OC_STACK_ERROR;

    if (m_ocResourcePtr == NULL)
        return result;

    if (oicResourceInit)
    {
        PostCallback attributeHandler = std::bind(&IotivityResourceClient::onPost, 
                                                  this, 
                                                  std::placeholders::_1,
                                                  std::placeholders::_2,
                                                  std::placeholders::_3);
        result = m_ocResourcePtr->post(oicResourceInit->m_resourceRep,
                                       QueryParamsMap(), 
                                       attributeHandler);
        if (OC_STACK_OK != result)
        {
            std::cerr << "post/create was unsuccessful\n";
            return result;
        }
    }

    return result;
}

OCStackResult IotivityResourceClient::retrieveResource() {

    OCStackResult result = OC_STACK_ERROR;

    if (m_ocResourcePtr == NULL)
        return result;

    GetCallback attributeHandler = std::bind(&IotivityResourceClient::onGet, 
                                             this, 
                                             std::placeholders::_1,
                                             std::placeholders::_2,
                                             std::placeholders::_3);
    result = m_ocResourcePtr->get(QueryParamsMap(), 
                                  attributeHandler);
    if (OC_STACK_OK != result)
    {
        std::cerr << "get was unsuccessful\n";
        return result;
    }
 
    return result;
}

OCStackResult IotivityResourceClient::updateResource() {

    OCStackResult result = OC_STACK_ERROR;

    if (m_ocResourcePtr == NULL)
        return result;

    OCRepresentation representation;
    //PostCallback ?? TODO
    PutCallback attributeHandler = std::bind(&IotivityResourceClient::onPut, 
                                             this, 
                                             std::placeholders::_1,
                                             std::placeholders::_2,
                                             std::placeholders::_3);
    
    result = m_ocResourcePtr->put(representation,
                                  QueryParamsMap(), 
                                  attributeHandler);
    if (OC_STACK_OK != result)
    {
        std::cerr << "update was unsuccessful\n";
        return result;
    }
 
    return result;
}

OCStackResult IotivityResourceClient::deleteResource() {

    OCStackResult result = OC_STACK_ERROR;

    if (m_ocResourcePtr == NULL)
        return result;

    DeleteCallback deleteHandler = std::bind(&IotivityResourceClient::onDelete, 
                                             this, 
                                             std::placeholders::_1,
                                             std::placeholders::_2);
    result = m_ocResourcePtr->deleteResource(deleteHandler);

    if (OC_STACK_OK != result)
    {
        std::cerr << "delete was unsuccessful\n";
        return result;
    }
 
    return result;
}

OCStackResult IotivityResourceClient::startObserving() {

    OCStackResult result = OC_STACK_ERROR;

    if (m_ocResourcePtr == NULL)
        return result;

    ObserveCallback observeHandler = std::bind(&IotivityResourceClient::onObserve, 
                                               this, 
                                               std::placeholders::_1,
                                               std::placeholders::_2,
                                               std::placeholders::_3,
                                               std::placeholders::_4);
       
    result = m_ocResourcePtr->observe(ObserveType::ObserveAll, 
                                      QueryParamsMap(), 
                                      observeHandler);
    if (OC_STACK_OK != result)
    {
        std::cerr << "observe was unsuccessful\n";
        return result;
    }
 
    return result;
}

OCStackResult IotivityResourceClient::cancelObserving() {

    OCStackResult result = OC_STACK_ERROR;

    if (m_ocResourcePtr == NULL)
        return result;

    GetCallback attributeHandler = std::bind(&IotivityResourceClient::onGet, 
                                             this, 
                                             std::placeholders::_1,
                                             std::placeholders::_2,
                                             std::placeholders::_3);
    result = m_ocResourcePtr->get(QueryParamsMap(), 
                                  attributeHandler);
    if (OC_STACK_OK != result)
    {
        std::cerr << "get was unsuccessful\n";
        return result;
    }
 
    return result;
}




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

    deserialize(value);
}

IotivityResourceInit::~IotivityResourceInit() {

}


void IotivityResourceInit::deserialize(const picojson::value& value) {

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
    picojson::object & propertiesobject = properties.get<picojson::object>();

    DEBUG_MSG("properties: size=%d\n", propertiesobject.size());

    for (picojson::value::object::iterator iter = propertiesobject.begin(); 
         iter != propertiesobject.end(); 
         ++iter) {
        
        std::string objectKey = iter->first;
        picojson::value objectValue = iter->second;
        if (objectValue.is<bool>())
        {
            DEBUG_MSG("[bool] key=%s, value=%d\n", objectKey.c_str(), objectValue.get<bool>());
            m_resourceRep[objectKey] = objectValue.get<bool>();
        }
        else if (objectValue.is<double>())
        {
            DEBUG_MSG("[double] key=%s, value=%f\n", objectKey.c_str(), objectValue.get<double>());
            m_resourceRep[objectKey] = objectValue.get<double>();
        }
        else if (objectValue.is<string>())
        {
            DEBUG_MSG("[string] key=%s, value=%s\n", objectKey.c_str(), objectValue.get<string>().c_str());
            m_resourceRep[objectKey] = objectValue.get<string>();
        }     
    }
}


void IotivityResourceInit::serialize(picojson::object& object) {

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
    picojson::value::object object;

    DEBUG_MSG("\n\n[Remote Client==>] entityHandlerCallback:\n");

    if (request)
    {
        ehResult = OC_EH_OK;
        IotivityRequestEvent iotivityRequestEvent;
        iotivityRequestEvent.deserialize(request);

        iotivityRequestEvent.m_resourceRepTarget = m_oicResourceInit->m_resourceRep;


        int requestFlag = request->getRequestHandlerFlag();
        if (requestFlag & RequestHandlerFlag::ObserverFlag)
        {
            DEBUG_MSG("postEntityHandler:ObserverFlag\n");

            iotivityRequestEvent.m_type = "observe";

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

       
 /*TODO for update representation
        if (iotivityRequestEvent.m_type == "update")
        {
            std::vector<std::string> foundPropertyNames;
            for (auto& cur: m_oicResourceInit->m_resourceRep)
            {
                std::string attrname = cur.attrname();

                if (std::find(iotivityRequestEvent.m_updatedPropertyNames.begin(), iotivityRequestEvent.m_updatedPropertyNames.end(), attrname) != iotivityRequestEvent.m_updatedPropertyNames.end())
                {
                   foundPropertyNames.push_back(attrname);


                   if (AttributeType::String == cur.type())
                   {
                       std::string curStr = cur.getValue<string>();
                       std::string newValue;
                       iotivityRequestEvent.m_resourceRep.getValue(attrname, newValue);
                       if (curStr != newValue)
                       {
                           m_oicResourceInit->m_resourceRep.setValue(attrname, newValue);
                           DEBUG_MSG("Updated[%s] old=%s, new=%s\n", attrname.c_str(), curStr.c_str(), newValue.c_str());
                       }
                   }
                   else if (AttributeType::Integer == cur.type())
                   {           
                       int intValue = cur.getValue<int>();
                       int newValue;
                       iotivityRequestEvent.m_resourceRep.getValue(attrname, newValue);
                       if (intValue != newValue)
                       {
                           m_oicResourceInit->m_resourceRep.setValue(attrname, newValue);
                           DEBUG_MSG("Updated[%s] old=%d, new=%d\n", attrname.c_str(), intValue, newValue);
                       }
                   }
                   else if (AttributeType::Double == cur.type())
                   {
                       double doubleValue = cur.getValue<double>();
                       double newValue;
                       iotivityRequestEvent.m_resourceRep.getValue(attrname, newValue);
                       if (doubleValue != newValue)
                       {
                           m_oicResourceInit->m_resourceRep.setValue(attrname, newValue);
                           DEBUG_MSG("Updated[%s] old=%f, new=%f\n", attrname.c_str(), doubleValue, newValue);
                       }
                   }           
                   else if (AttributeType::Boolean == cur.type())
                   {
                       bool boolValue = cur.getValue<bool>();
                       bool newValue;
                       iotivityRequestEvent.m_resourceRep.getValue(attrname, newValue);
                       if (boolValue != newValue)
                       {
                           m_oicResourceInit->m_resourceRep.setValue(attrname, newValue);
                           DEBUG_MSG("Updated[%s] old=%d, new=%d\n", attrname.c_str(), boolValue, newValue);
                       }
                   }
                }
            }

            iotivityRequestEvent.m_updatedPropertyNames = foundPropertyNames;
        }
*/
        object["cmd"] = picojson::value("entityHandler");
        picojson::object OicRequestEvent;
        iotivityRequestEvent.serialize(OicRequestEvent);
        object["OicRequestEvent"] = picojson::value(OicRequestEvent);

        picojson::value value(object);
        m_device->PostMessage(value.serialize().c_str());
    }
    else
    {
        std::cerr << "entityHandlerCallback: Request invalid" << std::endl;
    }

    return ehResult;
}

void IotivityResourceServer::serialize(picojson::object& object) {

    object["id"] = picojson::value((double)getResourceHandleToInt());

    picojson::object properties;
    m_oicResourceInit->serialize(properties);
    object["OicResourceInit"] = picojson::value(properties);
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

    DEBUG_MSG("registerResource handle=%d\n", m_resourceHandle);

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


void IotivityResourceClient::serialize(picojson::object& object) {

    object["id"] = picojson::value((double)getResourceHandleToInt());

    picojson::object properties;
    m_oicResourceInit->serialize(properties);
    object["OicResourceInit"] = picojson::value(properties);
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
    m_device->PostMessage(value.serialize().c_str());
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
    m_device->PostMessage(value.serialize().c_str());
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
    m_device->PostMessage(value.serialize().c_str());
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
    m_device->PostMessage(value.serialize().c_str());
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



IotivityRequestEvent::IotivityRequestEvent() {

}

IotivityRequestEvent::~IotivityRequestEvent() {

}

void IotivityRequestEvent::deserialize(std::shared_ptr<OCResourceRequest> request) {

    std::string requestType = request->getRequestType();
    int requestFlag = request->getRequestHandlerFlag();

    DEBUG_MSG("Deserialize: requestType=%s\n",requestType.c_str());
    DEBUG_MSG("Deserialize: requestFlag=0x%x\n",requestFlag);
    DEBUG_MSG("Deserialize: requestHandle=0x%x\n", (int)request->getRequestHandle());


    if(requestFlag & RequestHandlerFlag::RequestFlag)
    {
        if (requestType == "GET")
           m_type = "retrieve";
        else if (requestType == "PUT")
           m_type = "update";
        else if (requestType == "POST")
        {
           m_type = "create"; // or update TODO
           m_type = "update";
        }
        else if (requestType == "DELETE")
           m_type = "delete";

        m_resourceRep = request->getResourceRepresentation();
        PrintfOcRepresentation(m_resourceRep);
        for (auto& cur: m_resourceRep)
        {
            std::string attrname = cur.attrname();
            m_updatedPropertyNames.push_back(attrname);
        }
    }

    m_queries = request->getQueryParameters();
    if (!m_queries.empty())
    {
        for (auto it : m_queries)
        {
            DEBUG_MSG("Queries: key=%s, value=%s\n",it.first.c_str(), it.second.c_str());
        }
    }

    m_headerOptions = request->getHeaderOptions();
    if (!m_headerOptions.empty())
    {
        for (auto it = m_headerOptions.begin(); it != m_headerOptions.end(); ++it)
        {
            DEBUG_MSG("HeaderOptions: ID=%d, value=%s\n",it->getOptionID(), it->getOptionData().c_str());          
        }
    }

    m_requestId = (int)request->getRequestHandle();
    m_source = std::to_string((int)request->getRequestHandle());
    m_target = std::to_string((int)request->getResourceHandle());
}

void IotivityRequestEvent::deserialize(const picojson::value& value) {

    m_requestId = (int)value.get("requestId").get<double>();
    m_type = value.get("type").to_str();
    m_source = value.get("source").to_str();
    m_target = value.get("target").to_str();



    picojson::value properties = value.get("properties");
    picojson::object & propertiesobject = properties.get<picojson::object>();

    DEBUG_MSG("properties: size=%d\n", propertiesobject.size());

    for (picojson::value::object::iterator iter = propertiesobject.begin(); 
         iter != propertiesobject.end(); 
         ++iter) {
        
        std::string objectKey = iter->first;
        picojson::value objectValue = iter->second;
        if (objectValue.is<bool>())
        {
            DEBUG_MSG("[bool] key=%s, value=%d\n", objectKey.c_str(), objectValue.get<bool>());
            m_resourceRep[objectKey] = objectValue.get<bool>();
        }
        else if (objectValue.is<double>())
        {
            DEBUG_MSG("[double] key=%s, value=%f\n", objectKey.c_str(), objectValue.get<double>());
            m_resourceRep[objectKey] = objectValue.get<double>();
        }
        else if (objectValue.is<string>())
        {
            DEBUG_MSG("[string] key=%s, value=%s\n", objectKey.c_str(), objectValue.get<string>().c_str());
            m_resourceRep[objectKey] = objectValue.get<string>();
        }
    }

}


void IotivityRequestEvent::serialize(picojson::object& object) {

    object["type"] = picojson::value(m_type);
    object["requestId"] = picojson::value((double)m_requestId);
    object["source"] = picojson::value(m_source);
    object["target"] = picojson::value(m_target);



    if ((m_type == "create") || (m_type == "update") )
    {
        picojson::object properties;
        TranslateOCRepresentationToPicojson(m_resourceRep, properties);
        object["properties"] = picojson::value(properties);

        PrintfOcRepresentation(m_resourceRep);
    }

    if ((m_type == "retrieve") || (m_type == "observe"))
    {
        picojson::object properties;
        TranslateOCRepresentationToPicojson(m_resourceRepTarget, properties);
        object["properties"] = picojson::value(properties);

        PrintfOcRepresentation(m_resourceRepTarget);
    }

    if (m_type == "update")
    {
        picojson::array updatedPropertyNamesArray;
        for (int i = 0; i < m_updatedPropertyNames.size(); i++)
        {
            std::string propertyName = m_updatedPropertyNames[i];
            updatedPropertyNamesArray.push_back(picojson::value(propertyName));
        }
        object["updatedPropertyNames"] = picojson::value(updatedPropertyNamesArray);
    }

    picojson::array queryOptionsArray;
    if (!m_queries.empty())
    {
        for (auto it : m_queries)
        {
            picojson::object object;
            DEBUG_MSG("Queries: key=%s, value=%s\n",it.first.c_str(), it.second.c_str());
            object[it.first.c_str()] = picojson::value(it.second);
            queryOptionsArray.push_back(picojson::value(object));
        }
    }

    picojson::array headerOptionsArray;
    if (!m_headerOptions.empty())
    {
        picojson::object object;
        for (auto it = m_headerOptions.begin(); it != m_headerOptions.end(); ++it)
        {
            picojson::object object;
            DEBUG_MSG("HeaderOptions: ID=%d, value=%s\n",it->getOptionID(), it->getOptionData().c_str());   
            object[std::to_string(it->getOptionID()).c_str()] = picojson::value(it->getOptionData());
            headerOptionsArray.push_back(picojson::value(object));
        }
    }

}


OCStackResult IotivityRequestEvent::sendResponse() {

    OCStackResult result = OC_STACK_ERROR;

    auto pResponse = std::make_shared<OC::OCResourceResponse>();
    pResponse->setRequestHandle((void *)m_requestId);
    int targetId = atoi(m_target.c_str());
    pResponse->setResourceHandle((void *)targetId);

    DEBUG_MSG("handleSendResponse: type=%s\n", m_type.c_str());

    if (m_type == "retrieve")
    {
        // Send targetId representation
        pResponse->setResourceRepresentation(m_resourceRep);
        pResponse->setErrorCode(200);
        pResponse->setResponseResult(OC_EH_OK);
    }
    else if (m_type == "update")
    {
        pResponse->setResourceRepresentation(m_resourceRep);
        pResponse->setErrorCode(200);
        pResponse->setResponseResult(OC_EH_OK);
    }
    else if (m_type == "create") // POST (with special flags ??)
    {
        //OCRepresentation rep = pRequest->getResourceRepresentation();
        //OCRepresentation rep_post = post(rep);
        pResponse->setResourceRepresentation(m_resourceRep);
        pResponse->setErrorCode(200);
        pResponse->setResponseResult(OC_EH_OK);
    }
    else if (m_type == "observe")
    {
        pResponse->setResourceRepresentation(m_resourceRep);
        pResponse->setErrorCode(200);
        pResponse->setResponseResult(OC_EH_OK);
    }

    pResponse->setHeaderOptions(m_headerOptions);

    result = OCPlatform::sendResponse(pResponse);
    if (OC_STACK_OK != result)
    {
        std::cerr << "OCPlatform::sendResponse was unsuccessful\n";
    }

    return result;  
}

OCStackResult IotivityRequestEvent::sendError() {

    OCStackResult result = OC_STACK_ERROR;

    auto pResponse = std::make_shared<OC::OCResourceResponse>();
    pResponse->setRequestHandle((void *)m_requestId);
    int targetId = atoi(m_target.c_str());
    pResponse->setResourceHandle((void *)targetId);

    pResponse->setErrorCode(200 /* TODOvalue.get("error").to_str()*/);

    //pResponse->setHeaderOptions(m_headerOptions);

    result = OCPlatform::sendResponse(pResponse);
    if (OC_STACK_OK != result)
    {
        std::cerr << "OCPlatform::sendError was unsuccessful\n";
    }

    return result; 
}
 

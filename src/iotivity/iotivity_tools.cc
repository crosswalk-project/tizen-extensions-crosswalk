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
#include "iotivity/iotivity_tools.h"

char *pDebugEnv = NULL;


void PrintfOcResource(const OCResource & oCResource) {

    DEBUG_MSG("PrintfOcResource\n");
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

void PrintfOcRepresentation(const OCRepresentation & oCRepresentation) {

    DEBUG_MSG("PrintfOcRepresentation\n");
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
void TranslateOCRepresentationToPicojson(const OCRepresentation & oCRepresentation, picojson::object & objectRes) {

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
            double doubleValue = cur.getValue<double>();
            objectRes[attrname] = picojson::value((double)doubleValue);
        }
        else if (AttributeType::Boolean == cur.type())
        {
            bool boolValue = cur.getValue<bool>();
            objectRes[attrname] = picojson::value((bool)boolValue);
        }
    }
}

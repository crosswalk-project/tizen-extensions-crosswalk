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
#include "iotivity/iotivity_extension.h"
#include "iotivity/iotivity_instance.h"

common::Extension* CreateExtension() {
  return new IotivityExtension();
}

extern const char kSource_iotivity_api[];

IotivityExtension::IotivityExtension() {
  SetExtensionName("OIC");
  SetJavaScriptAPI(kSource_iotivity_api);
  const char* entry_points[] = {
    "iotivity.OicDevice",
    "iotivity.OicDeviceSettings",
    "iotivity.OicDeviceInfo",
    "iotivity.OicClient",
    "iotivity.OicServer",
    "iotivity.OicRequestEvent",
    "iotivity.OicDiscoveryOptions",
    "iotivity.OicResourceRepresentation",
    "iiotivity.OicResource",
    "iotivity.HeaderOption",
    "iotivity.QueryOption",
    NULL
  };
  SetExtraJSEntryPoints(entry_points);
}

IotivityExtension::~IotivityExtension() {}

common::Instance* IotivityExtension::CreateInstance() {
  return new IotivityInstance();
}


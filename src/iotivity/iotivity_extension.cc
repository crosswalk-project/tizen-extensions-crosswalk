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


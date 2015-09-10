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
#include "iotivity/iotivity_extension.h"
#include "iotivity/iotivity_instance.h"

common::Extension* CreateExtension() { return new IotivityExtension(); }

extern const char kSource_iotivity_api[];

IotivityExtension::IotivityExtension() {
  SetExtensionName("OIC");
  SetJavaScriptAPI(kSource_iotivity_api);
  const char* entry_points[] = {
      "iotivity.OicDevice", "iotivity.OicDeviceSettings",
      "iotivity.OicDeviceInfo", "iotivity.OicClient", "iotivity.OicServer",
      "iotivity.OicRequestEvent", "iotivity.OicDiscoveryOptions",
      "iotivity.OicResourceRepresentation", "iiotivity.OicResource",
      "iotivity.HeaderOption", "iotivity.QueryOption", NULL};
  SetExtraJSEntryPoints(entry_points);
}

IotivityExtension::~IotivityExtension() {}

common::Instance* IotivityExtension::CreateInstance() {
  return new IotivityInstance();
}

// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "network_bearer_selection/network_bearer_selection_extension.h"

#include "network_bearer_selection/network_bearer_selection_instance.h"

common::Extension* CreateExtension() {
  return new NetworkBearerSelectionExtension;
}

// This will be generated from network_bearer_selection_api.js.
extern const char kSource_network_bearer_selection_api[];

NetworkBearerSelectionExtension::NetworkBearerSelectionExtension() {
  const char* entry_points[] = { NULL };
  SetExtraJSEntryPoints(entry_points);
  SetExtensionName("tizen.networkbearerselection");
  SetJavaScriptAPI(kSource_network_bearer_selection_api);
}

NetworkBearerSelectionExtension::~NetworkBearerSelectionExtension() {}

common::Instance* NetworkBearerSelectionExtension::CreateInstance() {
  return new NetworkBearerSelectionInstance;
}

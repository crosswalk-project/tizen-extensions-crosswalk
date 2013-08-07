// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "network_bearer_selection/network_bearer_selection_context_desktop.h"

#include "network_bearer_selection/network_bearer_selection_request.h"
#include "common/extension_adapter.h"
#include "common/picojson.h"

CXWalkExtension* xwalk_extension_init(int32_t api_version) {
  return ExtensionAdapter<NetworkBearerSelectionContextDesktop>::Initialize();
}

NetworkBearerSelectionContextDesktop::NetworkBearerSelectionContextDesktop(
    ContextAPI* api) : NetworkBearerSelectionContext(api) {
}

void NetworkBearerSelectionContextDesktop::OnRequestRouteToHost(
    NetworkBearerSelectionRequest* request) {
  PostMessage(request->cmd(), NOT_SUPPORTED_ERR, false, request->reply_id());
  delete request;
}

void NetworkBearerSelectionContextDesktop::OnReleaseRouteToHost(
    NetworkBearerSelectionRequest* request) {
  PostMessage(request->cmd(), NOT_SUPPORTED_ERR, false, request->reply_id());
  delete request;
}

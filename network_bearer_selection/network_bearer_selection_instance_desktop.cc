// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "network_bearer_selection/network_bearer_selection_instance.h"

#include "common/picojson.h"
#include "network_bearer_selection/network_bearer_selection_request.h"

NetworkBearerSelectionInstance::NetworkBearerSelectionInstance() {
}

NetworkBearerSelectionInstance::~NetworkBearerSelectionInstance() {
}

void NetworkBearerSelectionInstance::OnRequestRouteToHost(
    NetworkBearerSelectionRequest* request) {
  InternalPostMessage(request->cmd(), NOT_SUPPORTED_ERR,
                      false, request->reply_id());
  delete request;
}

void NetworkBearerSelectionInstance::OnReleaseRouteToHost(
    NetworkBearerSelectionRequest* request) {
  InternalPostMessage(request->cmd(), NOT_SUPPORTED_ERR,
                      false, request->reply_id());
  delete request;
}

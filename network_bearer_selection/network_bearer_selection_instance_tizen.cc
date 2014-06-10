// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "network_bearer_selection/network_bearer_selection_instance.h"

#include "network_bearer_selection/network_bearer_selection_connection_tizen.h"
#include "network_bearer_selection/network_bearer_selection_request.h"

NetworkBearerSelectionInstance::NetworkBearerSelectionInstance() {
  connection_ = new NetworkBearerSelectionConnection();
}

NetworkBearerSelectionInstance::~NetworkBearerSelectionInstance() {
  delete connection_;
}

void NetworkBearerSelectionInstance::OnRequestRouteToHost(
    NetworkBearerSelectionRequest* request) {
  if (!connection_->is_valid()) {
    InternalPostMessage(request->cmd(), UNKNOWN_ERR,
                        false, request->reply_id());
    return;
  }

  connection_->RequestRouteToHost(request);
}

void NetworkBearerSelectionInstance::OnReleaseRouteToHost(
    NetworkBearerSelectionRequest* request) {
  if (!connection_->is_valid()) {
    InternalPostMessage(request->cmd(), UNKNOWN_ERR,
                        false, request->reply_id());
    return;
  }

  connection_->ReleaseRouteToHost(request);
}

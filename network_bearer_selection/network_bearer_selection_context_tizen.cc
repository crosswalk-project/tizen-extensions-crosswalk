// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "network_bearer_selection/network_bearer_selection_context_tizen.h"

#include "common/extension_adapter.h"
#include "network_bearer_selection/network_bearer_selection_request.h"
#include "network_bearer_selection/network_bearer_selection_connection_tizen.h"

DEFINE_XWALK_EXTENSION(NetworkBearerSelectionContextMobile);

NetworkBearerSelectionContextMobile::NetworkBearerSelectionContextMobile(
    ContextAPI* api) : NetworkBearerSelectionContext(api) {
  connection_ = new NetworkBearerSelectionConnection();
}

NetworkBearerSelectionContextMobile::~NetworkBearerSelectionContextMobile() {
  delete connection_;
}

void NetworkBearerSelectionContextMobile::OnRequestRouteToHost(
    NetworkBearerSelectionRequest* request) {
  if (!connection_->is_valid()) {
    PostMessage(request->cmd(), UNKNOWN_ERR, false, request->reply_id());
    return;
  }

  connection_->RequestRouteToHost(request);
}

void NetworkBearerSelectionContextMobile::OnReleaseRouteToHost(
    NetworkBearerSelectionRequest* request) {
  if (!connection_->is_valid()) {
    PostMessage(request->cmd(), UNKNOWN_ERR, false, request->reply_id());
    return;
  }

  connection_->ReleaseRouteToHost(request);
}

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "networkbearerselection/networkbearerselection_context_mobile.h"

#include "common/extension_adapter.h"
#include "networkbearerselection/networkbearerselection_request.h"
#include "networkbearerselection/networkbearerselection_connection.h"

CXWalkExtension* xwalk_extension_init(int32_t api_version) {
  return ExtensionAdapter<NetworkBearerSelectionContextMobile>::Initialize();
}

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

  PostMessage(request->cmd(), NO_ERROR, false, request->reply_id());
}

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NETWORK_BEARER_SELECTION_NETWORK_BEARER_SELECTION_CONTEXT_DESKTOP_H_
#define NETWORK_BEARER_SELECTION_NETWORK_BEARER_SELECTION_CONTEXT_DESKTOP_H_

#include "network_bearer_selection/network_bearer_selection_context.h"

class NetworkBearerSelectionContextDesktop
  : public NetworkBearerSelectionContext {
 public:
  explicit NetworkBearerSelectionContextDesktop(ContextAPI* api);

 private:
  virtual void OnRequestRouteToHost(NetworkBearerSelectionRequest* request);
  virtual void OnReleaseRouteToHost(NetworkBearerSelectionRequest* request);
};

#endif  // NETWORK_BEARER_SELECTION_NETWORK_BEARER_SELECTION_CONTEXT_DESKTOP_H_

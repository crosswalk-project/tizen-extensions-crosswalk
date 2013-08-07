// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NETWORKBEARERSELECTION_NETWORKBEARERSELECTION_CONTEXT_MOBILE_H_
#define NETWORKBEARERSELECTION_NETWORKBEARERSELECTION_CONTEXT_MOBILE_H_

#include "networkbearerselection/networkbearerselection_context.h"

class NetworkBearerSelectionConnection;

class NetworkBearerSelectionContextMobile
  : public NetworkBearerSelectionContext {
 public:
  explicit NetworkBearerSelectionContextMobile(ContextAPI* api);
  ~NetworkBearerSelectionContextMobile();

 private:
  virtual void OnRequestRouteToHost(NetworkBearerSelectionRequest* request);
  virtual void OnReleaseRouteToHost(NetworkBearerSelectionRequest* request);

  NetworkBearerSelectionConnection* connection_;
};

#endif  // NETWORKBEARERSELECTION_NETWORKBEARERSELECTION_CONTEXT_MOBILE_H_

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NETWORKBEARERSELECTION_NETWORKBEARERSELECTION_CONTEXT_DESKTOP_H_
#define NETWORKBEARERSELECTION_NETWORKBEARERSELECTION_CONTEXT_DESKTOP_H_

#include "networkbearerselection/networkbearerselection_context.h"

class NetworkBearerSelectionContextDesktop
  : public NetworkBearerSelectionContext {
 public:
  explicit NetworkBearerSelectionContextDesktop(ContextAPI* api);

 private:
  virtual void OnRequestRouteToHost(NetworkBearerSelectionRequest* request);
  virtual void OnReleaseRouteToHost(NetworkBearerSelectionRequest* request);
};

#endif  // NETWORKBEARERSELECTION_NETWORKBEARERSELECTION_CONTEXT_DESKTOP_H_

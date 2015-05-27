// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NETWORK_BEARER_SELECTION_NETWORK_BEARER_SELECTION_INSTANCE_H_
#define NETWORK_BEARER_SELECTION_NETWORK_BEARER_SELECTION_INSTANCE_H_

#include <string>

#include "common/extension.h"
#include "tizen/tizen.h"

class NetworkBearerSelectionRequest;
#ifdef TIZEN
class NetworkBearerSelectionConnection;
#endif

enum NetworkType {
  CELLULAR = 0,
  UNKNOWN = 1,
};

class NetworkBearerSelectionInstance : public common::Instance {
 public:
  NetworkBearerSelectionInstance();
  ~NetworkBearerSelectionInstance();

  void InternalPostMessage(const std::string& cmd,
                           WebApiAPIErrors error,
                           bool disconnected,
                           const std::string& reply_id);

 private:
  void HandleMessage(const char* msg);
  void HandleSyncMessage(const char* msg) {}

  void OnRequestRouteToHost(NetworkBearerSelectionRequest* request);
  void OnReleaseRouteToHost(NetworkBearerSelectionRequest* request);

#ifdef TIZEN
  NetworkBearerSelectionConnection* connection_;
#endif
};

#endif  // NETWORK_BEARER_SELECTION_NETWORK_BEARER_SELECTION_INSTANCE_H_

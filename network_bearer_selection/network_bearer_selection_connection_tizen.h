// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NETWORK_BEARER_SELECTION_NETWORK_BEARER_SELECTION_CONNECTION_TIZEN_H_
#define NETWORK_BEARER_SELECTION_NETWORK_BEARER_SELECTION_CONNECTION_TIZEN_H_

#include "network_bearer_selection/network_bearer_selection_instance.h"
#include <net_connection.h>

class NetworkBearerSelectionConnection {
 public:
  NetworkBearerSelectionConnection();
  ~NetworkBearerSelectionConnection();

  void RequestRouteToHost(NetworkBearerSelectionRequest* request);
  void ReleaseRouteToHost(NetworkBearerSelectionRequest* request);

  bool is_valid() { return is_valid_; }

 private:
  connection_profile_h GetProfileForNetworkType(NetworkType network_type);

  connection_h connection_;
  bool is_valid_;
};

#endif  // NETWORK_BEARER_SELECTION_NETWORK_BEARER_SELECTION_CONNECTION_TIZEN_H_

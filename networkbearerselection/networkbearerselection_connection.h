// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NETWORKBEARERSELECTION_NETWORKBEARERSELECTION_CONNECTION_H_
#define NETWORKBEARERSELECTION_NETWORKBEARERSELECTION_CONNECTION_H_

#include "networkbearerselection_context.h"
#include <net_connection.h>

class NetworkBearerSelectionConnection {
 public:
  NetworkBearerSelectionConnection();
  ~NetworkBearerSelectionConnection();

  void RequestRouteToHost(NetworkBearerSelectionRequest* request);

  bool is_valid() { return is_valid_; };

 private:
  connection_profile_h GetProfileForNetworkType(NetworkType network_type);

  connection_h connection_;
  bool is_valid_;
};

#endif  // NETWORKBEARERSELECTION_NETWORKBEARERSELECTION_CONNECTION_H_

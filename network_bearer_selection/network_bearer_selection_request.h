// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NETWORK_BEARER_SELECTION_NETWORK_BEARER_SELECTION_REQUEST_H_
#define NETWORK_BEARER_SELECTION_NETWORK_BEARER_SELECTION_REQUEST_H_

#include <string>
#include "network_bearer_selection/network_bearer_selection_instance.h"

class NetworkBearerSelectionRequest {
 public:
  explicit NetworkBearerSelectionRequest(
      NetworkBearerSelectionInstance* instance);

  void Success();
  void Disconnected();
  void Failure();

  std::string cmd() const { return cmd_; }
  std::string domain_name() const { return domain_name_; }
  NetworkType network_type() const { return network_type_; }
  std::string reply_id() const { return reply_id_; }

  void set_cmd(const std::string& cmd);
  void set_domain_name(const std::string& domain_name);
  void set_network_type(NetworkType network_type);
  void set_reply_id(const std::string& reply_id);

 private:
  NetworkBearerSelectionInstance* instance_;

  bool success_;

  std::string cmd_;
  std::string domain_name_;
  std::string reply_id_;
  NetworkType network_type_;
};

#endif  // NETWORK_BEARER_SELECTION_NETWORK_BEARER_SELECTION_REQUEST_H_

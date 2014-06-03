// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "network_bearer_selection/network_bearer_selection_request.h"

NetworkBearerSelectionRequest::NetworkBearerSelectionRequest(
    NetworkBearerSelectionInstance* instance)
  : instance_(instance), success_(false) {
}

void NetworkBearerSelectionRequest::Success() {
  // Tizen Network API has a inconsistence: it might synchronously
  // report that the profile state is "connected" at
  // connection_profile_get_state() before sending the callback of
  // the state change. This might make us call this method twice
  // in certain corner cases. The success_ guard prevents that.
  if (!success_)
    instance_->InternalPostMessage(cmd_, NO_ERROR, false, reply_id_);

  success_ = true;
}

void NetworkBearerSelectionRequest::Disconnected() {
  instance_->InternalPostMessage(cmd_, NO_ERROR, true, reply_id_);
}

void NetworkBearerSelectionRequest::Failure() {
  instance_->InternalPostMessage(cmd_, UNKNOWN_ERR, false, reply_id_);
}

void NetworkBearerSelectionRequest::set_cmd(const std::string& cmd) {
  cmd_ = cmd;
}

void NetworkBearerSelectionRequest::set_domain_name(
    const std::string& domain_name) {
  domain_name_ = domain_name;
}

void NetworkBearerSelectionRequest::set_network_type(NetworkType network_type) {
  network_type_ = network_type;
}

void NetworkBearerSelectionRequest::set_reply_id(const std::string& reply_id) {
  reply_id_ = reply_id;
}

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "network_bearer_selection/network_bearer_selection_instance.h"

#include "common/picojson.h"
#include "network_bearer_selection/network_bearer_selection_request.h"

void NetworkBearerSelectionInstance::InternalPostMessage(
    const std::string& cmd, WebApiAPIErrors error,
    bool disconnected, const std::string& reply_id) {
  picojson::value::object o;
  o["cmd"] = picojson::value(cmd);
  o["error"] = picojson::value(static_cast<double>(error));
  o["disconnected"] = picojson::value(disconnected);
  o["reply_id"] = picojson::value(reply_id);

  picojson::value v(o);
  PostMessage(v.serialize().c_str());
}

void NetworkBearerSelectionInstance::HandleMessage(const char* msg) {
  picojson::value v;
  std::string err;

  picojson::parse(v, msg, msg + strlen(msg), &err);
  if (!err.empty()) {
    std::cerr << ": invalid JSON message." << std::endl;
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  std::string domain_name = v.get("domain_name").to_str();
  std::string reply_id = v.get("reply_id").to_str();

  NetworkType network_type =
      static_cast<NetworkType>(v.get("network_type").get<double>());

  // FIXME(tmpsantos): The requests holds a reference to the context for
  // posting messages, which means it should outlive the context. We should
  // somehow make context refcounted to have this guarantee.
  NetworkBearerSelectionRequest* request =
      new NetworkBearerSelectionRequest(this);
  request->set_cmd(cmd);
  request->set_domain_name(domain_name);
  request->set_network_type(network_type);
  request->set_reply_id(reply_id);

  if (cmd == "requestRouteToHost")
    OnRequestRouteToHost(request);
  else if (cmd == "releaseRouteToHost")
    OnReleaseRouteToHost(request);
  else
    std::cerr << ": invalid command (" << cmd << ")." << std::endl;
}

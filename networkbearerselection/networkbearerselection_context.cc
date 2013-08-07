// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "networkbearerselection/networkbearerselection_context.h"

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "networkbearerselection/networkbearerselection_request.h"

extern const char kSource_networkbearerselection_api[];

NetworkBearerSelectionContext::NetworkBearerSelectionContext(ContextAPI* api)
    : api_(api) {
}

const char NetworkBearerSelectionContext::name[] = "tizen.networkbearerselection";

const char* NetworkBearerSelectionContext::GetJavaScript() {
  return kSource_networkbearerselection_api;
}

void NetworkBearerSelectionContext::PostMessage(const std::string& cmd,
                                                WebApiAPIErrors error,
                                                bool disconnected,
                                                const std::string& reply_id) {
  picojson::value::object o;
  o["cmd"] = picojson::value(cmd);
  o["error"] = picojson::value(static_cast<double>(error));
  o["disconnected"] = picojson::value(disconnected);
  o["reply_id"] = picojson::value(reply_id);

  picojson::value v(o);
  api_->PostMessage(v.serialize().c_str());
}

void NetworkBearerSelectionContext::HandleMessage(const char* message) {
  picojson::value v;
  std::string err;

  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cerr << name << ": invalid JSON message." << std::endl;
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
    std::cerr << name << ": invalid command (" << cmd << ")." << std::endl;
}

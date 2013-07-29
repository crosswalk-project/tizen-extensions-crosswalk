// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "networkbearerselection/networkbearerselection_context.h"

#include "common/picojson.h"

extern const char kSource_networkbearerselection_api[];

CXWalkExtension* xwalk_extension_init(int32_t api_version) {
  return ExtensionAdapter<NetworkBearerSelectionContext>::Initialize();
}

NetworkBearerSelectionContext::NetworkBearerSelectionContext(ContextAPI* api)
  : api_(api) {
}

const char NetworkBearerSelectionContext::name[] = "tizen.networkbearerselection";

const char* NetworkBearerSelectionContext::GetJavaScript() {
  return kSource_networkbearerselection_api;
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

  if (cmd == "requestRouteToHost")
    OnRequestRouteToHost(cmd, domain_name, network_type, reply_id);
  else if (cmd == "releaseRouteToHost")
    OnReleaseRouteToHost(cmd, domain_name, network_type, reply_id);
  else
    std::cerr << name << ": invalid command (" << cmd << ")." << std::endl;
}

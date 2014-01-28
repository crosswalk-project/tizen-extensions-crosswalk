// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen/tizen_extension.h"

// This will be generated from tizen_api.js.
extern const char kSource_tizen_api[];

common::Extension* CreateExtension() {
  return new TizenExtension;
}

TizenExtension::TizenExtension() {
  SetExtensionName("tizen");
  SetJavaScriptAPI(kSource_tizen_api);
}

TizenExtension::~TizenExtension() {}

common::Instance* TizenExtension::CreateInstance() {
  return new TizenExtensionInstance(this);
}

std::string TizenExtension::GetRuntimeVariable(const std::string& name) {
  char value_buffer[8192];
  GetRuntimeVariableInternal(name.c_str(), value_buffer, sizeof(value_buffer) - 1);
  return std::string(value_buffer);
}

void TizenExtensionInstance::HandleGetRuntimeVariable(
    const picojson::value& input) {
  if (!input.contains("var")) {
    SendSyncReply("{\"error\": 1}");
  } else {
    std::string var = input.get("var").to_str();
    SendSyncReply(extension_->GetRuntimeVariable(var).c_str());
  }
}

void TizenExtensionInstance::HandleSyncMessage(const char* msg) {
  picojson::value v;

  std::string err;
  picojson::parse(v, msg, msg + strlen(msg), &err);
  if (!err.empty()) {
    std::cerr << "Ignoring message.";
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "GetRuntimeVariable") {
    HandleGetRuntimeVariable(v);
  } else {
    std::cerr << "Ignoring unknown command: " << cmd;
  }
}

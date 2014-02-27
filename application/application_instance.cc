// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "application/application_instance.h"

#include "application/application_extension.h"
#include "application/application_information.h"

#include <memory>
#include <string>

ApplicationInstance::ApplicationInstance(ApplicationExtension* extension)
    : extension_(extension) {
}

ApplicationInstance::~ApplicationInstance() {
}

void ApplicationInstance::HandleMessage(const char* msg) {
  picojson::value v;

  std::string err;
  picojson::parse(v, msg, msg + strlen(msg), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "GetAppsInfo") {
    HandleGetAppsInfo(v);
  } else {
    std::cout << "ASSERT NOT REACHED.\n";
  }
}

void ApplicationInstance::HandleSyncMessage(const char* msg) {
  picojson::value v;
  std::string err;

  picojson::parse(v, msg, msg + strlen(msg), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "GetAppInfo") {
    HandleGetAppInfo(v);
  } else {
    std::cout << "ASSERT NOT REACHED.\n";
  }
}

void ApplicationInstance::HandleGetAppInfo(picojson::value& msg) {
  std::string app_id;
  if (msg.contains("id") && msg.get("id").is<std::string>())
    app_id = msg.get("id").to_str();
  else
    app_id = extension_->app_id();

  ApplicationInformation app_info(app_id);
  SendSyncReply(app_info.Serialize().c_str());
}

void ApplicationInstance::HandleGetAppsInfo(picojson::value& msg) {
  std::unique_ptr<picojson::value> result(
      ApplicationInformation::GetAllInstalled());
  ReturnMessageAsync(msg, result->get<picojson::object>());
}

void ApplicationInstance::ReturnMessageAsync(picojson::value& msg,
                                             const picojson::object& obj) {
  picojson::object& msg_obj = msg.get<picojson::object>();
  msg_obj.insert(obj.begin(), obj.end());
  PostMessage(msg.serialize().c_str());
}

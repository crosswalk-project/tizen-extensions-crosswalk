// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "application/application_instance.h"

#include <memory>
#include <iostream>
#include <string>
#include <sstream>

#include "application/application_manager.h"
#include "application/application_context.h"
#include "application/application_extension.h"
#include "application/application_information.h"

namespace {

const char kJSCbKey[] = "_callback";

double GetJSCallbackId(const picojson::value& msg) {
  assert(msg.contains(kJSCbKey));
  const picojson::value& id_value = msg.get(kJSCbKey);
  return id_value.get<double>();
}

void SetJSCallbackId(picojson::value& msg, double id) {
  assert(msg.is<picojson::object>());
  msg.get<picojson::object>()[kJSCbKey] = picojson::value(id);
}

}  // namespace

ApplicationInstance::ApplicationInstance(ApplicationManager* manager)
    : manager_(manager) {
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
  } else if (cmd == "GetAppsContext") {
    HandleGetAppsContext(v);
  } else if (cmd == "KillApp") {
    HandleKillApp(v);
  } else if (cmd == "LaunchApp") {
    HandleLaunchApp(v);
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
  } else if (cmd == "GetAppContext") {
    HandleGetAppContext(v);
  } else if (cmd == "GetCurrentApp") {
    HandleGetCurrentApp(v);
  } else if (cmd == "ExitCurrentApp") {
    HandleExitCurrentApp(v);
  } else if (cmd == "HideCurrentApp") {
    HandleHideCurrentApp(v);
  } else {
    std::cout << "ASSERT NOT REACHED.\n";
  }
}

void ApplicationInstance::HandleGetAppInfo(picojson::value& msg) {
  std::string app_id;
  if (msg.contains("id") && msg.get("id").is<std::string>())
    app_id = msg.get("id").to_str();
  else
    app_id = manager_->current_app_id();

  ApplicationInformation app_info(app_id);
  SendSyncReply(app_info.Serialize().c_str());
}

void ApplicationInstance::HandleGetAppContext(picojson::value& msg) {
  std::string ctx_id;
  if (msg.contains("id") && msg.get("id").is<std::string>()) {
    ctx_id = msg.get("id").to_str();
  } else {
    ctx_id = manager_->GetCurrentAppContextId();
  }

  ApplicationContext app_ctx(manager_, ctx_id);
  std::cerr << app_ctx.Serialize() << std::endl;
  SendSyncReply(app_ctx.Serialize().c_str());
}

void ApplicationInstance::HandleGetCurrentApp(picojson::value& msg) {
  std::string app_id = manager_->current_app_id();
  std::string ctx_id = manager_->GetCurrentAppContextId();
  ApplicationInformation app_info(app_id);
  ApplicationContext app_ctx(manager_, ctx_id);

  picojson::value result(picojson::object_type, true);
  result.get<picojson::object>()["appInfo"] =
      picojson::value(app_info.Serialize());
  result.get<picojson::object>()["appContext"] =
      picojson::value(app_ctx.Serialize());
  std::cerr << result.serialize() << std::endl;
  SendSyncReply(result.serialize().c_str());
}

void ApplicationInstance::HandleExitCurrentApp(picojson::value& msg) {
  picojson::value result = manager_->ExitCurrentApp();
  std::cerr << result.serialize() << std::endl;
  SendSyncReply(result.serialize().c_str());
}

void ApplicationInstance::HandleHideCurrentApp(picojson::value& msg) {
  picojson::value result = manager_->HideCurrentApp();
  SendSyncReply(result.serialize().c_str());
}

void ApplicationInstance::HandleGetAppsInfo(picojson::value& msg) {
  std::unique_ptr<picojson::object> result(
      ApplicationInformation::GetAllInstalled());
  ReturnMessageAsync(GetJSCallbackId(msg), *result);
}

void ApplicationInstance::HandleGetAppsContext(picojson::value& msg) {
  std::unique_ptr<picojson::object> result(
      ApplicationContext::GetAllRunning(manager_));
  ReturnMessageAsync(GetJSCallbackId(msg), *result);
}

void ApplicationInstance::HandleKillApp(picojson::value& msg) {
  std::string context_id = msg.get("id").to_str();
  AsyncMessageCallback callback =
      std::bind(&ApplicationInstance::ReturnMessageAsync,
                this, GetJSCallbackId(msg), std::placeholders::_1);
  manager_->KillApp(context_id, callback);
}

void ApplicationInstance::HandleLaunchApp(picojson::value& msg) {
  std::string app_id = msg.get("id").to_str();
  AsyncMessageCallback callback =
      std::bind(&ApplicationInstance::ReturnMessageAsync,
                this, GetJSCallbackId(msg), std::placeholders::_1);
  manager_->LaunchApp(app_id, callback);
}

void ApplicationInstance::ReturnMessageAsync(double callback_id,
                                             const picojson::object& obj) {
  picojson::value ret_msg(obj);
  SetJSCallbackId(ret_msg, callback_id);
  std::cerr << ret_msg.serialize() << std::endl;
  PostMessage(ret_msg.serialize().c_str());
}

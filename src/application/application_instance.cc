// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "application/application_instance.h"

#include <map>
#include <memory>
#include <iostream>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

#include "application/application.h"
#include "application/application_context.h"
#include "application/application_control.h"
#include "application/application_extension.h"
#include "application/application_information.h"
#include "application/application_manager.h"

namespace {

const char kJSCallbackKey[] = "_callback";
const char kJSReplyCallbackKey[] = "reply_callback";
const char kAppInfoEventCallback[] = "_appInfoEventCallback";
const char kEncodedBundle[] = "encoded_bundle";

// FIXME: This is forced by GetRuntimeVariable how actually works
// Therefore bundle is limited to some size  -> 1MB
const unsigned kEncodedBundleMaxLength = 1024 * 1024;

double GetJSCallbackId(const picojson::value& msg) {
  assert(msg.contains(kJSCallbackKey));
  const picojson::value& id_value = msg.get(kJSCallbackKey);
  return id_value.get<double>();
}

void SetJSCallbackId(picojson::value& msg, double id) {
  assert(msg.is<picojson::object>());
  msg.get<picojson::object>()[kJSCallbackKey] = picojson::value(id);
}

}  // namespace

ApplicationInstance::ApplicationInstance(ApplicationExtension* extension)
    : extension_(extension) {
}

ApplicationInstance::~ApplicationInstance() {
  ApplicationManager* manager = extension_->app_manager();
  if (manager->IsInstanceRegistered(this))
    manager->UnregisterAppInfoEvent(this);
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
  } else if (cmd == "LaunchAppControl") {
    HandleLaunchAppControl(v);
  } else if (cmd == "FindAppControl") {
    HandleFindAppControl(v);
  } else {
    std::cout << "ASSERT NOT REACHED. UNKNOWN COMMAND: " << cmd << std::endl;
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
    HandleGetCurrentApp();
  } else if (cmd == "ExitCurrentApp") {
    HandleExitCurrentApp();
  } else if (cmd == "HideCurrentApp") {
    HandleHideCurrentApp();
  } else if (cmd == "RegisterAppInfoEvent") {
    HandleRegisterAppInfoEvent();
  } else if (cmd == "UnregisterAppInfoEvent") {
    HandleUnregisterAppInfoEvent();
  } else if (cmd == "GetAppMetaData") {
    HandleGetAppMetaData(v);
  } else if (cmd == "ReplyResult") {
    HandleReplyResult(v);
  } else if (cmd == "ReplyFailure") {
    HandleReplyFailure();
  } else if (cmd == "GetRequestedAppControl") {
    HandleGetRequestedAppControl();
  } else {
    std::cout << "ASSERT NOT REACHED. UNKNOWN COMMAND: " << cmd << std::endl;
  }
}

void ApplicationInstance::HandleGetAppInfo(const picojson::value& msg) {
  if (msg.contains("id") && msg.get("id").is<std::string>()) {
    ApplicationInformation app_info(msg.get("id").to_str());
    SendSyncReply(app_info.Serialize().c_str());
  } else {
    Application* current_app = extension_->current_app();
    SendSyncReply(current_app->GetAppInfo().Serialize().c_str());
  }
}

void ApplicationInstance::HandleGetAppContext(const picojson::value& msg) {
  if (msg.contains("id") && msg.get("id").is<std::string>()) {
    ApplicationContext app_ctx(msg.get("id").to_str());
    SendSyncReply(app_ctx.Serialize().c_str());
  } else {
    Application* current_app = extension_->current_app();
    SendSyncReply(current_app->GetAppContext().Serialize().c_str());
  }
}

void ApplicationInstance::HandleGetCurrentApp() {
  SendSyncReply(extension_->current_app()->Serialize().c_str());
}

void ApplicationInstance::HandleExitCurrentApp() {
  std::unique_ptr<picojson::value> result(
      extension_->current_app()->Exit());
  SendSyncReply(result->serialize().c_str());
}

void ApplicationInstance::HandleHideCurrentApp() {
  std::unique_ptr<picojson::value> result(
      extension_->current_app()->Hide());
  SendSyncReply(result->serialize().c_str());
}

void ApplicationInstance::HandleRegisterAppInfoEvent() {
  auto post_event = std::bind(&ApplicationInstance::PostAppInfoEventMessage,
      this, std::placeholders::_1);
  std::unique_ptr<picojson::value> result(
      extension_->app_manager()->RegisterAppInfoEvent(this, post_event));
  SendSyncReply(result->serialize().c_str());
}

void ApplicationInstance::HandleUnregisterAppInfoEvent() {
  std::unique_ptr<picojson::value> result(
      extension_->app_manager()->UnregisterAppInfoEvent(this));
  SendSyncReply(result->serialize().c_str());
}

void ApplicationInstance::HandleGetAppMetaData(const picojson::value& msg) {
  std::string app_id;
  if (msg.contains("id") && msg.get("id").is<std::string>()) {
    app_id = msg.get("id").to_str();
  } else {
    app_id = extension_->current_app()->GetAppId();
  }

  std::unique_ptr<picojson::value> result(
      extension_->app_manager()->GetAppMetaData(app_id));
  SendSyncReply(result->serialize().c_str());
}

void ApplicationInstance::HandleGetRequestedAppControl() {
  std::unique_ptr<picojson::value> result(
      extension_->app_manager()->GetRequestedAppControl(
          extension_->GetRuntimeVariable(kEncodedBundle,
              kEncodedBundleMaxLength)));
  SendSyncReply(result->serialize().c_str());
}

void ApplicationInstance::HandleReplyResult(const picojson::value& msg) {
  if (!msg.contains("data") || !msg.get("data").is<picojson::array>()) {
    SendSyncReplyInternalError();
    return;
  }

  const picojson::array& array = msg.get("data").get<picojson::array>();

  std::vector<std::unique_ptr<ApplicationControlData> > data;

  for (const auto& item : array) {
    if (!item.contains("key") ||
        !item.get("key").is<std::string>() ||
        !item.contains("value") ||
        !item.get("value").is<picojson::array>()) {
      SendSyncReplyInternalError();
      return;
    }
    const picojson::array& array =
        item.get("value").get<picojson::array>();
    std::unique_ptr<std::vector<std::string>> values(
        new std::vector<std::string>());
    for (const auto& value : array) {
      if (!value.is<std::string>()) {
        SendSyncReplyInternalError();
        return;
      }
      values->push_back(value.get<std::string>());
    }

    std::unique_ptr<ApplicationControlData> app_control_data(
        new ApplicationControlData(
            item.get("key").get<std::string>(),
            std::move(values)));

    data.push_back(std::move(app_control_data));
  }

  std::unique_ptr<picojson::value> result(
      extension_->app_manager()->ReplyResult(data,
         extension_->GetRuntimeVariable(kEncodedBundle,
             kEncodedBundleMaxLength)));
  SendSyncReply(result->serialize().c_str());
}

void ApplicationInstance::HandleReplyFailure() {
  std::unique_ptr<picojson::value> result(
      extension_->app_manager()->ReplyFailure(
          extension_->GetRuntimeVariable(kEncodedBundle,
              kEncodedBundleMaxLength)));
  SendSyncReply(result->serialize().c_str());
}

void ApplicationInstance::HandleGetAppsInfo(const picojson::value& msg) {
  std::unique_ptr<picojson::value> result(
      ApplicationInformation::GetAllInstalled());
  ReturnMessageAsync(GetJSCallbackId(msg), *result);
}

void ApplicationInstance::HandleGetAppsContext(const picojson::value& msg) {
  std::unique_ptr<picojson::value> result(ApplicationContext::GetAllRunning());
  ReturnMessageAsync(GetJSCallbackId(msg), *result);
}

void ApplicationInstance::HandleKillApp(const picojson::value& msg) {
  std::string context_id = msg.get("id").to_str();
  std::unique_ptr<picojson::value> result(
      extension_->app_manager()->KillApp(context_id));
  ReturnMessageAsync(GetJSCallbackId(msg), *result);
}

void ApplicationInstance::HandleLaunchApp(const picojson::value& msg) {
  std::string app_id = msg.get("id").to_str();
  std::unique_ptr<picojson::value> result(
      extension_->app_manager()->LaunchApp(app_id));
  ReturnMessageAsync(GetJSCallbackId(msg), *result);
}

void ApplicationInstance::HandleLaunchAppControl(const picojson::value& msg) {
  std::string app_id;
  if (msg.contains("id") && msg.get("id").is<std::string>())
    app_id = msg.get("id").get<std::string>();
  if (!msg.contains(kJSReplyCallbackKey) ||
      !msg.get(kJSReplyCallbackKey).is<double>()) {
    ReturnMessageAsync(GetJSCallbackId(msg), *GetUnknownErrorResult());
    return;
  }
  int reply_callback_id = static_cast<int>(
      msg.get(kJSReplyCallbackKey).get<double>());

  if (!msg.contains("appControl")) {
    ReturnMessageAsync(GetJSCallbackId(msg), *GetUnknownErrorResult());
    return;
  }
  std::unique_ptr<ApplicationControl> control =
      ApplicationControl::ApplicationControlFromJSON(msg.get("appControl"));
  if (!control) {
    ReturnMessageAsync(GetJSCallbackId(msg), *GetUnknownErrorResult());
    return;
  }

  std::unique_ptr<picojson::value> result(
      extension_->app_manager()->LaunchAppControl(*control, app_id,
          reply_callback_id,
          [this](picojson::value& value, int reply_id) {
            ReturnMessageAsync(reply_id, value);
          }));

  ReturnMessageAsync(GetJSCallbackId(msg), *result);
}

void ApplicationInstance::HandleFindAppControl(const picojson::value& msg) {
  if (!msg.contains("appControl")) {
    ReturnMessageAsync(GetJSCallbackId(msg), *GetUnknownErrorResult());
  }
  std::unique_ptr<ApplicationControl> control =
      ApplicationControl::ApplicationControlFromJSON(msg.get("appControl"));
  if (!control) {
    ReturnMessageAsync(GetJSCallbackId(msg), *GetUnknownErrorResult());
    return;
  }

  std::unique_ptr<picojson::value> result(
      extension_->app_manager()->FindAppControl(*control));
  ReturnMessageAsync(GetJSCallbackId(msg), *result);
}

void ApplicationInstance::ReturnMessageAsync(double callback_id,
                                             picojson::value& value) {
  SetJSCallbackId(value, callback_id);
  PostMessage(value.serialize().c_str());
}

void ApplicationInstance::PostAppInfoEventMessage(picojson::object& events) {
  events[kJSCallbackKey] = picojson::value(kAppInfoEventCallback);
  PostMessage(picojson::value(events).serialize().c_str());
}

void ApplicationInstance::SendSyncReplyInternalError() {
  SendSyncReply(GetUnknownErrorResult()->serialize().c_str());
}

std::unique_ptr<picojson::value> ApplicationInstance::GetUnknownErrorResult() {
  picojson::object result;
  result.insert(std::make_pair("error",
      picojson::value(static_cast<double>(WebApiAPIErrors::UNKNOWN_ERR))));
  return std::unique_ptr<picojson::value>(new picojson::value(result));
}

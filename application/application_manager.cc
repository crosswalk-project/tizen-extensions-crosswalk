// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "application/application_manager.h"

#include <iostream>

#include "application/application_dbus_agent.h"
#include "application/application_information.h"

ApplicationManager* ApplicationManager::Create(const std::string& pkg_id) {
  std::string app_id = ApplicationInformation::PkgIdToAppId(pkg_id);
  if (app_id.empty()) {
    std::cerr << "Can't translate app package ID to application ID.\n";
    return NULL;
  }

  ApplicationDBusAgent* dbus_agent = ApplicationDBusAgent::Create(pkg_id);
  if (!dbus_agent) {
    std::cerr << "Can't create app DBus agent.\n";
    return NULL;
  }

  return new ApplicationManager(pkg_id, app_id, dbus_agent);
}

ApplicationManager::ApplicationManager(const std::string& pkg_id,
                                       const std::string& app_id,
                                       ApplicationDBusAgent* dbus_agent)
    : curr_pkg_id_(pkg_id),
      curr_app_id_(app_id),
      dbus_agent_(dbus_agent) {
}

ApplicationManager::~ApplicationManager() {
}

const std::string& ApplicationManager::GetCurrentAppContextId() {
  if (!curr_context_id_.empty())
    return curr_context_id_;

  curr_context_id_ = "-1";
  GVariant* value = dbus_agent_->GetLauncherPid();
  if (value) {
    int pid;
    g_variant_get(value, "(i)", &pid);
    g_variant_unref(value);
    if (pid <= 0)
      std::cerr << "Get invalid launcher pid: " << pid << std::endl;
    else
      curr_context_id_ = std::to_string(pid);
  }

  return curr_context_id_;
}

void ApplicationManager::GetAppIdByPid(int pid,
                                       std::string& app_id,
                                       WebApiAPIErrors& error) {
  GVariant* value = dbus_agent_->GetAppIdByPid(pid);
  if (!value) {
    error = WebApiAPIErrors::UNKNOWN_ERR;
    return;
  }

  gchar* app_id_str;
  g_variant_get(value, "(i&s)", &error, &app_id_str);
  app_id = app_id_str;
  g_variant_unref(value);
}

picojson::value ApplicationManager::ExitCurrentApp() {
  GVariant* value = dbus_agent_->ExitCurrentApp();
  // TODO(xiang): parse result to JSON
  std::cerr << "ASSERT NOT IMPLEMENTED.\n";
  return picojson::value();
}

picojson::value ApplicationManager::HideCurrentApp() {
  GVariant* value = dbus_agent_->HideCurrentApp();
  // TODO(xiang): parse result to JSON
  std::cerr << "ASSERT NOT IMPLEMENTED.\n";
  return picojson::value();
}

void ApplicationManager::KillApp(const std::string& context_id,
                                 AsyncMessageCallback callback) {
  AppDBusCallback dbus_callback =
      std::bind(&ApplicationManager::HandleDBusResult,
                this, callback, std::placeholders::_1);
  dbus_agent_->KillApp(context_id, dbus_callback);
}

void ApplicationManager::LaunchApp(const std::string& app_id,
                                   AsyncMessageCallback callback) {
  AppDBusCallback dbus_callback =
      std::bind(&ApplicationManager::HandleDBusResult,
                this, callback, std::placeholders::_1);
  dbus_agent_->LaunchApp(app_id, dbus_callback);
}

picojson::object ApplicationManager::HandleDBusResult(
    AsyncMessageCallback callback,
    GVariant* value) {
  gint error;
  g_variant_get(value, "(i)", &error);
  picojson::object obj = picojson::object();

  if (static_cast<WebApiAPIErrors>(error) != WebApiAPIErrors::NO_ERROR) {
    picojson::object error_obj = picojson::object();
    error_obj["code"] = picojson::value(static_cast<double>(error));
    obj["error"] = picojson::value(error_obj);
  }
  callback(obj);
}

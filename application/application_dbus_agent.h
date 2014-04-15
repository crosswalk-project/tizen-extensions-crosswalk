// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPLICATION_APPLICATION_DBUS_AGENT_H_
#define APPLICATION_APPLICATION_DBUS_AGENT_H_

#include <gio/gio.h>

#include <string>

#include "application/application_manager.h"

using AppDBusCallback = ApplicationManager::AppDBusCallback;

class ApplicationDBusAgent {
 public:
  static ApplicationDBusAgent* Create(const std::string& xwalk_app_id);

  GVariant* GetLauncherPid();
  GVariant* GetAppIdByPid(int pid);
  GVariant* ExitCurrentApp();
  GVariant* HideCurrentApp();

  void LaunchApp(const std::string& app_id, AppDBusCallback callback);
  void KillApp(const std::string& context_id, AppDBusCallback callback);

 private:
  ApplicationDBusAgent(GDBusProxy* runtime_app_proxy,
                       GDBusProxy* launcher_app_proxy);

  GDBusProxy* runtime_proxy_;
  GDBusProxy* launcher_proxy_;
};

#endif  // APPLICATION_APPLICATION_DBUS_AGENT_H_

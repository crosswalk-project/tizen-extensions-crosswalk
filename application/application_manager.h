// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPLICATION_APPLICATION_MANAGER_H_
#define APPLICATION_APPLICATION_MANAGER_H_

#include <gio/gio.h>

#include <functional>
#include <memory>
#include <string>

#include "application/application_instance.h"
#include "tizen/tizen.h"

class ApplicationDBusAgent;

using AsyncMessageCallback = ApplicationInstance::AsyncMessageCallback;

class ApplicationManager {
 public:
  typedef std::function<picojson::object(GVariant* v)> AppDBusCallback;

  static ApplicationManager* Create(const std::string& pkg_id);

  ~ApplicationManager();

  const std::string& GetCurrentAppContextId();
  const std::string& current_app_id() const { return curr_app_id_; }
  const std::string& current_pkg_id() const { return curr_pkg_id_; }

  void GetAppIdByPid(int pid, std::string& app_id, WebApiAPIErrors& error);
  picojson::value ExitCurrentApp();
  picojson::value HideCurrentApp();
  void KillApp(const std::string& context_id, AsyncMessageCallback callback);
  void LaunchApp(const std::string& app_id, AsyncMessageCallback callback);

 private:
  ApplicationManager(const std::string& pkg_id,
                     const std::string& app_id,
                     ApplicationDBusAgent* dbus_agent);

  // Handle async DBus callback, the value will be an integer Tizen error code.
  picojson::object HandleDBusResult(AsyncMessageCallback callback,
                                    GVariant* value);

  std::string curr_app_id_;
  std::string curr_pkg_id_;
  std::string curr_context_id_;
  std::unique_ptr<ApplicationDBusAgent> dbus_agent_;
};

#endif  // APPLICATION_APPLICATION_MANAGER_H_

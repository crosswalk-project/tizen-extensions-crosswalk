// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPLICATION_APPLICATION_MANAGER_H_
#define APPLICATION_APPLICATION_MANAGER_H_

#include <package_manager.h>
#include <package-manager.h>

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "common/picojson.h"

class ApplicationInstance;

class ApplicationManager {
 public:
  ApplicationManager();
  ~ApplicationManager();

  picojson::value* KillApp(const std::string& context_id);
  picojson::value* LaunchApp(const std::string& app_id);
  picojson::value* GetAppMetaData(const std::string& app_id);

  // Application information events registration/unregistration handlers.
  typedef std::function<void(picojson::object&)> AppInfoEventCallback;
  picojson::value* RegisterAppInfoEvent(
      ApplicationInstance* instance,
      const AppInfoEventCallback& event_callback);
  picojson::value* UnregisterAppInfoEvent(ApplicationInstance* instance);
  bool IsInstanceRegistered(ApplicationInstance* const instance) const;

  void OnAppInfoEvent(const std::string& event_type,
                      const std::string& event_state,
                      const std::vector<std::string>& app_ids);

 private:
  pkgmgr_client* pkgmgr_handle_;
  std::string started_event_;
  std::vector<std::string> uninstalled_app_ids_;

  typedef std::map<ApplicationInstance*, AppInfoEventCallback>
      AppInfoEventInstanceMap;
  AppInfoEventInstanceMap instances_;
};

#endif  // APPLICATION_APPLICATION_MANAGER_H_

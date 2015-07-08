// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPLICATION_APPLICATION_MANAGER_H_
#define APPLICATION_APPLICATION_MANAGER_H_

#include <app_service.h>
#include <package-manager.h>

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "common/picojson.h"
#include "application/application_control.h"
#include "application/application_control_data.h"

class ApplicationInstance;

class ApplicationManager {
 public:
  typedef std::function<void(picojson::value&, int)> ReplyCallback;

  ApplicationManager();
  ~ApplicationManager();

  picojson::value* KillApp(const std::string& context_id);
  picojson::value* LaunchApp(const std::string& app_id);
  picojson::value* LaunchAppControl(const ApplicationControl& app_control,
      const std::string& app_id, int reply_callback_id,
      ReplyCallback post_callback);
  picojson::value* FindAppControl(const ApplicationControl& app_control);
  picojson::value* GetAppMetaData(const std::string& app_id);
  picojson::value* GetRequestedAppControl(const std::string& encoded_bundle);
  picojson::value* ReplyResult(
      const std::vector<std::unique_ptr<ApplicationControlData> >& data,
      const std::string& encoded_bundle);
  picojson::value* ReplyFailure(const std::string& encoded_bundle);

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

  typedef std::tuple<int, ReplyCallback, ApplicationManager*> CallbackData;
  typedef std::unique_ptr<CallbackData> CallbackDataPtr;
  typedef std::map<ApplicationInstance*, AppInfoEventCallback>
      AppInfoEventInstanceMap;
  AppInfoEventInstanceMap instances_;
  std::map<int, CallbackDataPtr> reply_callbacks_;
};

#endif  // APPLICATION_APPLICATION_MANAGER_H_

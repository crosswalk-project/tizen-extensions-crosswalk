// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPLICATION_APPLICATION_INSTANCE_H_
#define APPLICATION_APPLICATION_INSTANCE_H_

#include <functional>

#include "common/extension.h"
#include "common/picojson.h"

class ApplicationManager;

class ApplicationInstance : public common::Instance {
 public:
  typedef std::function<void(const picojson::object&)> AsyncMessageCallback;

  explicit ApplicationInstance(ApplicationManager* current_app);
  ~ApplicationInstance();

 private:
  // common::Instance implementation.
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* msg);

  // Synchronous message handlers.
  void HandleGetAppInfo(picojson::value& msg);
  void HandleGetAppContext(picojson::value& msg);
  void HandleGetCurrentApp(picojson::value& msg);
  void HandleExitCurrentApp(picojson::value& msg);
  void HandleHideCurrentApp(picojson::value& msg);

  // Asynchronous message handlers.
  void HandleGetAppsInfo(picojson::value& msg);
  void HandleGetAppsContext(picojson::value& msg);
  void HandleKillApp(picojson::value& msg);
  void HandleLaunchApp(picojson::value& msg);

  void ReturnMessageAsync(double callback_id, const picojson::object& obj);

  ApplicationManager* manager_;
};

#endif  // APPLICATION_APPLICATION_INSTANCE_H_

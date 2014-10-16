// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPLICATION_APPLICATION_INSTANCE_H_
#define APPLICATION_APPLICATION_INSTANCE_H_

#include <functional>
#include <memory>

#include "common/extension.h"
#include "common/picojson.h"

class ApplicationExtension;

class ApplicationInstance : public common::Instance {
 public:
  explicit ApplicationInstance(ApplicationExtension* extension);
  ~ApplicationInstance();

 private:
  // common::Instance implementation.
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* msg);

  // Synchronous message handlers.
  void HandleGetAppInfo(const picojson::value& msg);
  void HandleGetAppContext(const picojson::value& msg);
  void HandleGetCurrentApp();
  void HandleExitCurrentApp();
  void HandleHideCurrentApp();
  void HandleRegisterAppInfoEvent();
  void HandleUnregisterAppInfoEvent();
  void HandleGetAppMetaData(const picojson::value& msg);
  void HandleGetRequestedAppControl();
  void HandleReplyResult(const picojson::value& msg);
  void HandleReplyFailure();

  // Asynchronous message handlers.
  void HandleGetAppsInfo(const picojson::value& msg);
  void HandleGetAppsContext(const picojson::value& msg);
  void HandleKillApp(const picojson::value& msg);
  void HandleLaunchApp(const picojson::value& msg);
  void HandleLaunchAppControl(const picojson::value& msg);
  void HandleFindAppControl(const picojson::value& msg);

  void ReturnMessageAsync(double callback_id, picojson::value& value);
  void PostAppInfoEventMessage(picojson::object& events);
  void SendSyncReplyInternalError();
  std::unique_ptr<picojson::value> GetUnknownErrorResult();

  ApplicationExtension* extension_;
};

#endif  // APPLICATION_APPLICATION_INSTANCE_H_

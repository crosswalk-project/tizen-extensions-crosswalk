// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ALARM_ALARM_INSTANCE_H_
#define ALARM_ALARM_INSTANCE_H_

#include <memory>

#include "common/extension.h"
#include "common/picojson.h"

class AlarmManager;

class AlarmInstance : public common::Instance {
 public:
  AlarmInstance();
  virtual ~AlarmInstance();

 private:
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* msg);

  void HandleAddAbsoluteAlarm(const picojson::value& msg);
  void HandleAddRelativeAlarm(const picojson::value& msg);
  void HandleGetRemainingSeconds(const picojson::value& msg);
  void HandleGetNextScheduledDate(const picojson::value& msg);
  void HandleGetAlarm(const picojson::value& msg);
  void HandleGetAllAlarms();
  void HandleRemoveAlarm(const picojson::value& msg);
  void HandleRemoveAllAlarms();

  void SendSyncMessage(int err_code, const picojson::value& data);

  std::unique_ptr<AlarmManager> alarm_manager_;
};

#endif  // ALARM_ALARM_INSTANCE_H_

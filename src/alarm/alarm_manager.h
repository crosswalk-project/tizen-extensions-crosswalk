// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ALARM_ALARM_MANAGER_H_
#define ALARM_ALARM_MANAGER_H_

#include <app_control.h>
#include <memory>
#include <string>
#include <vector>

class AlarmInfo;

class AlarmManager {
 public:
  AlarmManager();
  ~AlarmManager();

  void SetHostAppId(const std::string &host_app_id);
  int ScheduleAlarm(const std::string& app_id, AlarmInfo* alarm) const;
  int GetNextScheduledDate(int alarm_id, int& output) const;
  int GetRemainingSeconds(int alarm_id, int& output) const;
  int RemoveAlarm(int alarm_id) const;
  int RemoveAllAlarms() const;
  int GetAlarm(int alarm_id, AlarmInfo* alarm) const;
  int GetAllAlarms(std::vector<std::shared_ptr<AlarmInfo> >& output) const;

 private:
  app_control_h CreateAppLaunchAppControl(const std::string& app_id) const;
  void DestroyAppControl(app_control_h app_control) const;
  int StoreAlarmInAppControl(app_control_h app_control,
                             AlarmInfo* alarm) const;
  int RestoreAlarmFromAppControl(app_control_h app_control,
                                 AlarmInfo* alarm) const;
  bool CheckOwnership(int alarm_id) const;
  bool CheckOwnership(app_control_h app_control) const;

  std::string host_app_id_;
};

#endif  // ALARM_ALARM_MANAGER_H_

// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ALARM_ALARM_MANAGER_H_
#define ALARM_ALARM_MANAGER_H_

#include <app_service.h>
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
  service_h CreateAppLaunchService(const std::string& app_id) const;
  void DestroyService(service_h service) const;
  int StoreAlarmInService(service_h service, AlarmInfo* alarm) const;
  int RestoreAlarmFromService(service_h service, AlarmInfo* alarm) const;
  bool CheckOwnership(int alarm_id) const;
  bool CheckOwnership(service_h service) const;

  std::string host_app_id_;
};

#endif  // ALARM_ALARM_MANAGER_H_

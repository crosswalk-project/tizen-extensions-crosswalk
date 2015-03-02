// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "alarm/alarm_manager.h"

#include <app.h>
#include <iostream>

#include "alarm/alarm_info.h"

namespace {

const char kAlarmInfoKey[] = "app_control.extra.alarm";
const char kHostAppIdKey[] = "app_control.extra.hostappid";

bool AlarmIterateCallback(int id, void* userdata) {
  std::vector<int>* ids = reinterpret_cast<std::vector<int>*>(userdata);
  ids->push_back(id);
  return true;
}

}  // namespace

AlarmManager::AlarmManager() {}

AlarmManager::~AlarmManager() {}

void AlarmManager::SetHostAppId(const std::string &host_app_id) {
  host_app_id_ = host_app_id;
}

int AlarmManager::ScheduleAlarm(const std::string& app_id,
                                AlarmInfo* alarm) const {
  if (!alarm)
    return -1;

  app_control_h app_control = CreateAppLaunchAppControl(app_id);
  if (!app_control)
    return APP_CONTROL_RESULT_FAILED;

  int ret = StoreAlarmInAppControl(app_control, alarm);
  if (ret) {
    DestroyAppControl(app_control);
    std::cerr << "Failed to store the alarm information." << std::endl;
    return ret;
  }

  ret = app_control_add_extra_data(app_control, kHostAppIdKey,
      host_app_id_.c_str());
  if (ret) {
    DestroyAppControl(app_control);
    std::cerr << "Failed to store host application ID." << std::endl;
    return ret;
  }

  int alarm_id = 0;
  switch (alarm->type()) {
    case AlarmInfo::AlarmType::ABSOLUTE: {
      time_t tval = static_cast<time_t>(alarm->date());
      struct tm date_tm = {0};
      localtime_r(&tval, &date_tm);
      if (alarm->weekflag()) {
        ret = alarm_schedule_with_recurrence_week_flag(app_control, &date_tm,
            alarm->weekflag(), &alarm_id);
      } else {
        ret = alarm_schedule_at_date(app_control, &date_tm, alarm->period(),
            &alarm_id);
      }
      break;
    }
    case AlarmInfo::AlarmType::RELATIVE:
      ret = alarm_schedule_after_delay(app_control, alarm->delay(),
          alarm->period(), &alarm_id);
      break;
    default:
      std::cerr << "Wrong alarm type." << std::endl;
      ret = -1;
      break;
  }

  alarm->SetId(alarm_id);
  DestroyAppControl(app_control);

  if (ret) {
    std::cerr << "Failed to schedule an alarm." << std::endl;
    return ret;
  }

  return 0;
}

int AlarmManager::GetNextScheduledDate(int alarm_id, int& output) const {
  if (!CheckOwnership(alarm_id))
    return -1;

  struct tm date_tm;
  int ret = alarm_get_scheduled_date(alarm_id, &date_tm);
  if (ret)
    return ret;

  struct tm cur_date_tm;
  ret = alarm_get_current_time(&cur_date_tm);
  if (ret)
    return ret;

  int next = mktime(&date_tm);
  if (next < mktime(&cur_date_tm))
    return -1;

  output = next;
  return 0;
}

int AlarmManager::GetRemainingSeconds(int alarm_id, int& output) const {
  if (!CheckOwnership(alarm_id))
    return -1;

  struct tm date_tm;
  int ret = alarm_get_scheduled_date(alarm_id, &date_tm);
  if (ret)
    return ret;

  struct tm cur_date_tm;
  ret = alarm_get_current_time(&cur_date_tm);
  if (ret)
    return ret;

  int next = static_cast<int>(mktime(&date_tm));
  int current = static_cast<int>(mktime(&cur_date_tm));
  if (next < current)
    return -1;

  output = next - current;
  return 0;
}

int AlarmManager::RemoveAlarm(int alarm_id) const {
  app_control_h app_control = 0;
  int ret = alarm_get_app_control(alarm_id, &app_control);
  if (ret)
    return ret;

  if (!CheckOwnership(app_control)) {
    DestroyAppControl(app_control);
    return -1;
  }

  ret = alarm_cancel(alarm_id);
  DestroyAppControl(app_control);
  return ret;
}

int AlarmManager::RemoveAllAlarms() const {
  std::vector<int> alarm_ids;
  int ret = alarm_foreach_registered_alarm(AlarmIterateCallback, &alarm_ids);
  if (ret)
    return ret;

  for (int i = 0; i < alarm_ids.size(); i++) {
    RemoveAlarm(alarm_ids[i]);
  }
  return 0;
}

int AlarmManager::GetAlarm(int alarm_id, AlarmInfo* alarm) const {
  if (!alarm)
    return -1;

  app_control_h app_control = NULL;
  int ret = alarm_get_app_control(alarm_id, &app_control);
  if (ret)
    return ret;

  if (!CheckOwnership(app_control)) {
    DestroyAppControl(app_control);
    return -1;
  }

  ret = RestoreAlarmFromAppControl(app_control, alarm);
  alarm->SetId(alarm_id);

  DestroyAppControl(app_control);
  return ret;
}

int AlarmManager::GetAllAlarms(
    std::vector<std::shared_ptr<AlarmInfo> >& output) const {
  std::vector<int> alarm_ids;
  int ret = alarm_foreach_registered_alarm(AlarmIterateCallback, &alarm_ids);
  if (ret)
    return ret;

  for (int i = 0; i < alarm_ids.size(); i++) {
    std::shared_ptr<AlarmInfo> alarm(new AlarmInfo());
    if (GetAlarm(alarm_ids[i], alarm.get()) == 0)
      output.push_back(alarm);
  }

  return 0;
}

app_control_h
AlarmManager::CreateAppLaunchAppControl(const std::string& app_id) const {
  app_control_h app_control = 0;
  if (app_control_create(&app_control)) {
    std::cerr << "Failed to call app_control_create()" << std::endl;
    return 0;
  }

  app_control_set_app_id(app_control, app_id.c_str());
  app_control_set_operation(app_control, APP_CONTROL_OPERATION_DEFAULT);
  return app_control;
}

void AlarmManager::DestroyAppControl(app_control_h app_control) const {
  if (app_control_destroy(app_control))
    std::cerr << "Failed to call app_control_destroy()" << std::endl;
}

int AlarmManager::StoreAlarmInAppControl(app_control_h app_control,
                                         AlarmInfo* alarm) const {
  if (!app_control || !alarm)
    return -1;

  return app_control_add_extra_data(app_control, kAlarmInfoKey,
      alarm->Serialize().c_str());
}

int AlarmManager::RestoreAlarmFromAppControl(app_control_h app_control,
                                             AlarmInfo* alarm) const {
  if (!app_control || !alarm)
    return -1;

  char* buffer = NULL;
  int ret = app_control_get_extra_data(app_control, kAlarmInfoKey, &buffer);
  if (ret)
    return ret;

  if (!alarm->Deserialize(buffer)) {
    ret = -1;
  }

  free(buffer);
  return ret;
}

// Return true if the alarm is created by this application.
// Otherwise return false.
bool AlarmManager::CheckOwnership(int alarm_id) const {
  app_control_h app_control = NULL;
  int ret = alarm_get_app_control(alarm_id, &app_control);
  if (ret)
    return false;

  bool passed = CheckOwnership(app_control);
  DestroyAppControl(app_control);

  return passed;
}

bool AlarmManager::CheckOwnership(app_control_h app_control) const {
  char* buffer = NULL;
  if (!app_control ||
      app_control_get_extra_data(app_control, kHostAppIdKey, &buffer))
    return false;

  bool passed = (host_app_id_ == std::string(buffer));
  free(buffer);

  return passed;
}

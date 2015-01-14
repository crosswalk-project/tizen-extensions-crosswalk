// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "alarm/alarm_manager.h"

#include <app.h>
#include <iostream>

#include "alarm/alarm_info.h"

namespace {

const char kAlarmInfoKey[] = "service.extra.alarm";
const char kHostAppIdKey[] = "service.extra.hostappid";

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

  service_h service = CreateAppLaunchService(app_id);
  if (!service)
    return SERVICE_RESULT_FAILED;

  int ret = StoreAlarmInService(service, alarm);
  if (ret) {
    DestroyService(service);
    std::cerr << "Failed to store the alarm information." << std::endl;
    return ret;
  }

  ret = service_add_extra_data(service, kHostAppIdKey, host_app_id_.c_str());
  if (ret) {
    DestroyService(service);
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
        ret = alarm_schedule_with_recurrence_week_flag(service, &date_tm,
            alarm->weekflag(), &alarm_id);
      } else {
        ret = alarm_schedule_at_date(service, &date_tm, alarm->period(),
            &alarm_id);
      }
      break;
    }
    case AlarmInfo::AlarmType::RELATIVE:
      ret = alarm_schedule_after_delay(service, alarm->delay(),
          alarm->period(), &alarm_id);
      break;
    default:
      std::cerr << "Wrong alarm type." << std::endl;
      ret = -1;
      break;
  }

  alarm->SetId(alarm_id);
  DestroyService(service);

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
  service_h service = 0;
  int ret = alarm_get_service(alarm_id, &service);
  if (ret)
    return ret;

  if (!CheckOwnership(service)) {
    DestroyService(service);
    return -1;
  }

  ret = alarm_cancel(alarm_id);
  DestroyService(service);
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

  service_h service = NULL;
  int ret = alarm_get_service(alarm_id, &service);
  if (ret)
    return ret;

  if (!CheckOwnership(service)) {
    DestroyService(service);
    return -1;
  }

  ret = RestoreAlarmFromService(service, alarm);
  alarm->SetId(alarm_id);

  DestroyService(service);
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

service_h
AlarmManager::CreateAppLaunchService(const std::string& app_id) const {
  service_h service = 0;
  if (service_create(&service)) {
    std::cerr << "Failed to call service_create()" << std::endl;
    return 0;
  }

  service_set_app_id(service, app_id.c_str());
  service_set_operation(service, SERVICE_OPERATION_DEFAULT);
  return service;
}

void AlarmManager::DestroyService(service_h service) const {
  if (service_destroy(service))
    std::cerr << "Failed to call service_destroy()" << std::endl;
}

int AlarmManager::StoreAlarmInService(service_h service,
                                      AlarmInfo* alarm) const {
  if (!service || !alarm)
    return -1;

  return service_add_extra_data(service, kAlarmInfoKey,
      alarm->Serialize().c_str());
}

int AlarmManager::RestoreAlarmFromService(service_h service,
                                          AlarmInfo* alarm) const {
  if (!service || !alarm)
    return -1;

  char* buffer = NULL;
  int ret = service_get_extra_data(service, kAlarmInfoKey, &buffer);
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
  service_h service = NULL;
  int ret = alarm_get_service(alarm_id, &service);
  if (ret)
    return false;

  bool passed = CheckOwnership(service);
  DestroyService(service);

  return passed;
}

bool AlarmManager::CheckOwnership(service_h service) const {
  char* buffer = NULL;
  if (!service || service_get_extra_data(service, kHostAppIdKey, &buffer))
    return false;

  bool passed = (host_app_id_ == std::string(buffer));
  free(buffer);

  return passed;
}

// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "alarm/alarm_instance.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "alarm/alarm_info.h"
#include "alarm/alarm_manager.h"

namespace {

const char cmdAddAbsAlarm[] = "alarm.add.absolute";
const char cmdAddRelAlarm[] = "alarm.add.relative";
const char cmdGetRemainingSec[] = "alarm.get.remainingsec";
const char cmdGetNextScheduledDate[] = "alarm.get.nextScheduledDate";
const char cmdGetAlarm[] = "alarm.getInfo";
const char cmdGetAllAlarms[] = "alarm.getAllInfo";
const char cmdRemoveAlarm[] = "alarm.remove";
const char cmdRemoveAllAlarm[] = "alarm.removeAll";

}  // namespace

AlarmInstance::AlarmInstance() {
  alarm_manager_.reset(new AlarmManager());

  // Store the host application Id into the manager.
  std::string id_str = common::Extension::GetRuntimeVariable("app_id", 64);
  std::istringstream buf(id_str);
  picojson::value id_val;
  picojson::parse(id_val, buf);
  alarm_manager_->SetHostAppId(id_val.get<std::string>());
}

AlarmInstance::~AlarmInstance() {}

void AlarmInstance::HandleMessage(const char* msg) {
  picojson::value msg_obj;
  std::string err;
  picojson::parse(msg_obj, msg, msg + strlen(msg), &err);
  if (!err.empty()) {
    std::cerr << "Failed to parse the message." << std::endl;
    return;
  }
}

void AlarmInstance::HandleSyncMessage(const char* msg) {
  picojson::value msg_obj;
  std::string err;
  picojson::parse(msg_obj, msg, msg + strlen(msg), &err);
  if (!err.empty()) {
    std::cerr << "Failed to parse the sync message: " << msg << std::endl;
    return;
  }

  std::string cmdName = msg_obj.get("cmd").to_str();
  if (cmdName == cmdAddAbsAlarm)
    HandleAddAbsoluteAlarm(msg_obj);
  else if (cmdName == cmdAddRelAlarm)
    HandleAddRelativeAlarm(msg_obj);
  else if (cmdName == cmdGetRemainingSec)
    HandleGetRemainingSeconds(msg_obj);
  else if (cmdName == cmdGetNextScheduledDate)
    HandleGetNextScheduledDate(msg_obj);
  else if (cmdName == cmdGetAlarm)
    HandleGetAlarm(msg_obj);
  else if (cmdName == cmdGetAllAlarms)
    HandleGetAllAlarms();
  else if (cmdName == cmdRemoveAlarm)
    HandleRemoveAlarm(msg_obj);
  else if (cmdName == cmdRemoveAllAlarm)
    HandleRemoveAllAlarms();
}

void AlarmInstance::HandleAddAbsoluteAlarm(const picojson::value& msg) {
  std::string app_id = msg.get("applicationId").to_str();
  picojson::value alarm_value = msg.get("alarm");
  int dateval = static_cast<int>(alarm_value.get("date").get<double>());
  int period = static_cast<int>(alarm_value.get("period").get<double>());
  int week_flag = static_cast<int>(alarm_value.get("daysOfTheWeek")
      .get<double>());

  std::unique_ptr<AlarmInfo> alarm(new AlarmInfo(0, AlarmInfo::ABSOLUTE,
      dateval, 0, period, week_flag));

  int ret = alarm_manager_->ScheduleAlarm(app_id, alarm.get());

  if (!ret) {
    picojson::object obj;
    obj["id"] = picojson::value(static_cast<double>(alarm->id()));
    SendSyncMessage(0, picojson::value(obj));
  } else {
    SendSyncMessage(ret,
        picojson::value(std::string("Failed to add alarm")));
  }
}

void AlarmInstance::HandleAddRelativeAlarm(const picojson::value& msg) {
  std::string app_id = msg.get("applicationId").to_str();
  picojson::value alarm_value = msg.get("alarm");
  int delay = static_cast<int>(alarm_value.get("delay").get<double>());
  int period = static_cast<int>(alarm_value.get("period").get<double>());

  std::unique_ptr<AlarmInfo> alarm(new AlarmInfo(0, AlarmInfo::RELATIVE,
      0, delay, period, 0));

  int ret = alarm_manager_->ScheduleAlarm(app_id, alarm.get());

  if (!ret) {
    picojson::object obj;
    obj["id"] = picojson::value(static_cast<double>(alarm->id()));
    SendSyncMessage(0, picojson::value(obj));
  } else {
    SendSyncMessage(ret,
        picojson::value(std::string("Failed to add alarm")));
  }
}

void AlarmInstance::HandleGetRemainingSeconds(const picojson::value& msg) {
  int alarm_id = static_cast<int>(msg.get("alarm").get<double>());
  int seconds = 0;

  int ret = alarm_manager_->GetRemainingSeconds(alarm_id, seconds);

  if (!ret) {
    picojson::object obj;
    obj["remainingSeconds"] = picojson::value(static_cast<double>(seconds));
    SendSyncMessage(ret, picojson::value(obj));
  } else {
    SendSyncMessage(ret, picojson::value(std::string("")));
  }
}

void AlarmInstance::HandleGetNextScheduledDate(const picojson::value& msg) {
  int alarm_id = static_cast<int>(msg.get("alarm").get<double>());
  int date = 0;

  int ret = alarm_manager_->GetNextScheduledDate(alarm_id, date);

  if (!ret) {
    picojson::object obj;
    obj["nextScheduledDate"] = picojson::value(static_cast<double>(date));
    SendSyncMessage(ret, picojson::value(obj));
  } else {
    SendSyncMessage(ret, picojson::value(std::string("")));
  }
}

void AlarmInstance::HandleGetAlarm(const picojson::value& msg) {
  int alarm_id = static_cast<int>(msg.get("alarm").get<double>());
  std::unique_ptr<AlarmInfo> alarm(new AlarmInfo());

  int ret = alarm_manager_->GetAlarm(alarm_id, alarm.get());

  if (!ret) {
    SendSyncMessage(0, picojson::value(alarm->Serialize()));
  } else {
    SendSyncMessage(ret, picojson::value(std::string("Not found.")));
  }
}

void AlarmInstance::HandleGetAllAlarms() {
  std::vector<std::shared_ptr<AlarmInfo> > alarms;
  int ret = alarm_manager_->GetAllAlarms(alarms);
  picojson::array alarm_infos;
  if (!ret) {
    for (int i = 0; i < alarms.size(); i++) {
      alarm_infos.push_back(picojson::value(alarms[i]->Serialize()));
    }
  }

  SendSyncMessage(ret, picojson::value(alarm_infos));
}

void AlarmInstance::HandleRemoveAlarm(const picojson::value& msg) {
  int alarm_id = static_cast<int>(msg.get("alarm").get<double>());
  int ret = alarm_manager_->RemoveAlarm(alarm_id);
  SendSyncMessage(ret, picojson::value(std::string("")));
}

void AlarmInstance::HandleRemoveAllAlarms() {
  int ret = alarm_manager_->RemoveAllAlarms();
  SendSyncMessage(ret, picojson::value(std::string("")));
}

void AlarmInstance::SendSyncMessage(int err_code, const picojson::value& data) {
  picojson::object obj;
  if (err_code)
    obj["error"] = picojson::value(static_cast<double>(err_code));
  obj["data"] = data;
  picojson::value val(obj);
  SendSyncReply(val.serialize().c_str());
}

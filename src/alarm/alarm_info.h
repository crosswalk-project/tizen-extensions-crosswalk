// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ALARM_ALARM_INFO_H_
#define ALARM_ALARM_INFO_H_

#include <string>

class AlarmInfo {
 public:
  enum AlarmType {
    ABSOLUTE,
    RELATIVE
  };

  AlarmInfo() {}
  AlarmInfo(int id, AlarmType type, int date,
            int delay, int period, int weekflag)
    : id_(id), type_(type), date_(date), delay_(delay),
      period_(period), weekflag_(weekflag) {}

  ~AlarmInfo() {}

  int id() const { return id_; }
  void SetId(int id) { id_ = id; }
  AlarmType type() const { return type_; }
  int date() const { return date_; }
  int delay() const { return delay_; }
  int period() const { return period_; }
  int weekflag() const { return weekflag_; }

  std::string Serialize();
  bool Deserialize(const char* stream);

 private:
  int id_;
  AlarmType type_;
  int date_;
  int delay_;
  int period_;
  int weekflag_;
};

#endif  // ALARM_ALARM_INFO_H_

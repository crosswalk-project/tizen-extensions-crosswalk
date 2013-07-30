// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_BATTERY_H_
#define SYSTEM_INFO_SYSTEM_INFO_BATTERY_H_

#include <libudev.h>

#include "common/picojson.h"
#include "common/utils.h"

class SysInfoBattery {
 public:
  static SysInfoBattery& GetSysInfoBattery(picojson::value& error) {
    static SysInfoBattery instance(error);
    return instance;
  }
  ~SysInfoBattery();
  void Update(picojson::value& error, picojson::value& data);

 private:
  SysInfoBattery(picojson::value& error);

  struct udev* udev_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoBattery);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_BATTERY_H_

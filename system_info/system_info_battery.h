// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_BATTERY_H_
#define SYSTEM_INFO_SYSTEM_INFO_BATTERY_H_

#include <glib.h>
#include <libudev.h>

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"

class SysInfoBattery {
 public:
  static SysInfoBattery& GetSysInfoBattery(ContextAPI* api) {
    static SysInfoBattery instance(api);
    return instance;
  }
  ~SysInfoBattery();
  void Get(picojson::value& error, picojson::value& data);
  inline void StartListen() {
    g_timeout_add_seconds(3,
                          SysInfoBattery::TimedOutUpdate,
                          static_cast<gpointer>(this));
    stopping_ = false;
  }
  inline void StopListen() { stopping_ = true; }

 private:
  SysInfoBattery(ContextAPI* api);

  static gboolean TimedOutUpdate(gpointer user_data);
  bool Update(picojson::value& error);
  void SetData(picojson::value& data);

  ContextAPI* api_;
  struct udev* udev_;
  double level_;
  bool charging_;
  bool stopping_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoBattery);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_BATTERY_H_

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
#include "system_info/system_info_utils.h"

class SysInfoBattery {
 public:
  static SysInfoBattery& GetSysInfoBattery(ContextAPI* api) {
    static SysInfoBattery instance(api);
    return instance;
  }
  ~SysInfoBattery();
  void Get(picojson::value& error, picojson::value& data);
  inline void StartListen() {
    // FIXME(halton): Use udev D-Bus interface to monitor.
    g_timeout_add(system_info::default_timeout_interval,
                  SysInfoBattery::OnUpdateTimeout,
                  static_cast<gpointer>(this));
    stopping_ = false;
  }
  inline void StopListen() { stopping_ = true; }

 private:
  explicit SysInfoBattery(ContextAPI* api);

  static gboolean OnUpdateTimeout(gpointer user_data);
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

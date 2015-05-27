// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_BATTERY_H_
#define SYSTEM_INFO_SYSTEM_INFO_BATTERY_H_

#include <glib.h>
#include <libudev.h>

#if defined(TIZEN)
#include <vconf.h>
#include <vconf-keys.h>
#endif

#include <string>

#include "common/picojson.h"
#include "common/utils.h"
#include "system_info/system_info_instance.h"
#include "system_info/system_info_utils.h"

class SysInfoBattery : public SysInfoObject {
 public:
  static SysInfoObject& GetInstance() {
    static SysInfoBattery instance;
    return instance;
  }

  ~SysInfoBattery();
  void Get(picojson::value& error, picojson::value& data);
  void StartListening();
  void StopListening();

  static const std::string name_;

 private:
  SysInfoBattery();
  bool Update(picojson::value& error);
  void SetData(picojson::value& data);

#if defined(GENERIC_DESKTOP)
  static gboolean OnUpdateTimeout(gpointer user_data);

  udev* udev_;
  int timeout_cb_id_;
#elif defined(TIZEN)
  void UpdateLevel(double level);
  void UpdateCharging(bool charging);
  static void OnLevelChanged(keynode_t* node, void* user_data);
  static void OnIsChargingChanged(keynode_t* node, void* user_data);

#endif

  double level_;
  bool charging_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoBattery);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_BATTERY_H_

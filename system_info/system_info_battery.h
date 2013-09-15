// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_BATTERY_H_
#define SYSTEM_INFO_SYSTEM_INFO_BATTERY_H_

#include <glib.h>
#include <libudev.h>

#if defined(TIZEN_MOBILE)
#include <vconf.h>
#include <vconf-keys.h>
#endif

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"
#include "system_info/system_info_utils.h"

class SysInfoBattery {
 public:
  explicit SysInfoBattery(ContextAPI* api);
  ~SysInfoBattery();
  void Get(picojson::value& error, picojson::value& data); //NOLINT
  void StartListening();
  void StopListening();

 private:
  bool Update(picojson::value& error); //NOLINT
  void SetData(picojson::value& data); //NOLINT

#if defined(GENERIC_DESKTOP)
  static gboolean OnUpdateTimeout(gpointer user_data);

  int timeout_cb_id_;
#elif defined(TIZEN_MOBILE)
  void UpdateLevel(double level);
  void UpdateCharging(bool charging);
  static void OnLevelChanged(keynode_t* node, void* user_data);
  static void OnIsChargingChanged(keynode_t* node, void* user_data);

  bool is_registered_;
#endif

  ContextAPI* api_;
  struct udev* udev_;
  double level_;
  bool charging_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoBattery);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_BATTERY_H_

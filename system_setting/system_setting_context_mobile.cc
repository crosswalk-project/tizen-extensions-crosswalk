// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_setting/system_setting_context.h"

#include <system_settings.h>
#include <vconf.h>
#include "common/picojson.h"

void SystemSettingContext::HandleSetProperty(const picojson::value& msg) {
  SystemSettingType type = static_cast<SystemSettingType>
    (msg.get("_type").get<double>());
  const char* value = msg.get("_file").to_str().c_str();
  const char* reply_id = msg.get("_reply_id").to_str().c_str();
  system_settings_key_e key;
  switch (type) {
    case HOME_SCREEN:
      key = SYSTEM_SETTINGS_KEY_WALLPAPER_HOME_SCREEN;
      break;
    case LOCK_SCREEN:
      key = SYSTEM_SETTINGS_KEY_WALLPAPER_LOCK_SCREEN;
      break;
    case INCOMING_CALL:
      key = SYSTEM_SETTINGS_KEY_INCOMING_CALL_RINGTONE;
      break;
    case NOTIFICATION_EMAIL:
      key = SYSTEM_SETTINGS_KEY_EMAIL_ALERT_RINGTONE;
      break;
  default:
    std::cout<< "Invalid Key : should not reach here";
    break;
  }

  int ret = system_settings_set_value_string(key, value);
  OnPropertyHandled(reply_id, value, ret);
}

void SystemSettingContext::HandleGetProperty(const picojson::value& msg) {
  SystemSettingType type = static_cast<SystemSettingType>
    (msg.get("_type").get<double>());
  const char* reply_id = msg.get("_reply_id").to_str().c_str();
  system_settings_key_e key;
  switch (type) {
    case HOME_SCREEN:
      key = SYSTEM_SETTINGS_KEY_WALLPAPER_HOME_SCREEN;
      break;
    case LOCK_SCREEN:
      key = SYSTEM_SETTINGS_KEY_WALLPAPER_LOCK_SCREEN;
      break;
    case INCOMING_CALL:
      key = SYSTEM_SETTINGS_KEY_INCOMING_CALL_RINGTONE;
      break;
    case NOTIFICATION_EMAIL:
      key = SYSTEM_SETTINGS_KEY_EMAIL_ALERT_RINGTONE;
      break;
  default:
    std::cout<< "Invalid Key :should not reach here";
    break;
  }

  char* value = NULL;
  int ret = system_settings_get_value_string(key, &value);
  OnPropertyHandled(reply_id, value, ret);
  free(value);
}

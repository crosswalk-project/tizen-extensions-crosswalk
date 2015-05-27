// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_setting/system_setting_instance.h"
#include "system_setting/system_setting_locale.h"

#include <system_settings.h>
#include <vconf.h>
#include "common/picojson.h"

void SystemSettingInstance::HandleSetProperty(const picojson::value& msg) {
  SystemSettingType type = static_cast<SystemSettingType>
    (msg.get("_type").get<double>());
  const char* value = msg.get("_file").to_str().c_str();
  const char* reply_id = msg.get("_reply_id").to_str().c_str();
  system_settings_key_e key = SYSTEM_SETTINGS_KEY_INCOMING_CALL_RINGTONE;
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
    case LOCALE:
      system_setting::setLocale(value);
      break;
    default:
      std::cout<< "Invalid Key : should not reach here";
      break;
  }

  int ret = 0;
  if (type != LOCALE)
    ret = system_settings_set_value_string(key, value);
  OnPropertyHandled(reply_id, value, ret);
}

void SystemSettingInstance::HandleGetProperty(const picojson::value& msg) {
  SystemSettingType type = static_cast<SystemSettingType>
    (msg.get("_type").get<double>());
  const char* reply_id = msg.get("_reply_id").to_str().c_str();
  system_settings_key_e key = SYSTEM_SETTINGS_KEY_INCOMING_CALL_RINGTONE;
  char* value = NULL;
  std::string locale_str;
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
    case LOCALE:
      locale_str = system_setting::getLocale();
      value = reinterpret_cast<char *>(malloc(locale_str.length() + 1));
      std::snprintf(value, sizeof(value), locale_str.c_str());
      break;
    default:
      std::cout<< "Invalid Key :should not reach here";
      break;
  }

  int ret = 0;
  if (type != LOCALE)
    ret = system_settings_get_value_string(key, &value);
  OnPropertyHandled(reply_id, value, ret);
  free(value);
}

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_locale.h"

#include <stdlib.h>

#include "common/picojson.h"

SysInfoLocale::SysInfoLocale() {
  pthread_mutex_init(&events_list_mutex_, NULL);
}

void SysInfoLocale::StartListening(ContextAPI* api) {
  AutoLock lock(&events_list_mutex_);
  local_events_.push_back(api);

  if (local_events_.size() > 1)
    return;

  vconf_notify_key_changed(VCONFKEY_REGIONFORMAT,
      static_cast<vconf_callback_fn>(OnCountryChanged), this);
  vconf_notify_key_changed(VCONFKEY_LANGSET,
      static_cast<vconf_callback_fn>(OnLanguageChanged), this);
}

void SysInfoLocale::StopListening(ContextAPI* api) {
  AutoLock lock(&events_list_mutex_);
  local_events_.remove(api);

  if (!local_events_.empty())
    return;

  vconf_ignore_key_changed(VCONFKEY_REGIONFORMAT,
      static_cast<vconf_callback_fn>(OnCountryChanged));
  vconf_ignore_key_changed(VCONFKEY_LANGSET,
      static_cast<vconf_callback_fn>(OnLanguageChanged));
}

void SysInfoLocale::OnLanguageChanged(keynode_t* node, void* user_data) {
  char* language = vconf_keynode_get_str(node);
  SysInfoLocale* locale = static_cast<SysInfoLocale*>(user_data);

  if (language != locale->language_) {
    locale->language_ = std::string(language);
    locale->Update();
  }

  free(language);
}

void SysInfoLocale::OnCountryChanged(keynode_t* node, void* user_data) {
  char* country = vconf_keynode_get_str(node);
  SysInfoLocale* locale = static_cast<SysInfoLocale*>(user_data);

  if (country != locale->country_) {
    locale->country_ = std::string(country);
    locale->Update();
  }

  free(country);
}

void SysInfoLocale::Get(picojson::value& error,
                       picojson::value& data) {
  // language
  if (!GetLanguage()) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get language failed."));
    return;
  }
  system_info::SetPicoJsonObjectValue(data, "language",
      picojson::value(language_));

  // timezone
  if (!GetCountry()) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get timezone failed."));
    return;
  }
  system_info::SetPicoJsonObjectValue(data, "country",
      picojson::value(country_));

  system_info::SetPicoJsonObjectValue(error, "message",
      picojson::value(""));
}

void SysInfoLocale::Update() {
  picojson::value output = picojson::value(picojson::object());
  picojson::value data = picojson::value(picojson::object());


  system_info::SetPicoJsonObjectValue(data, "language",
      picojson::value(language_));
  system_info::SetPicoJsonObjectValue(data, "country",
      picojson::value(country_));

  system_info::SetPicoJsonObjectValue(output, "cmd",
      picojson::value("SystemInfoPropertyValueChanged"));
  system_info::SetPicoJsonObjectValue(output, "prop",
      picojson::value("LOCALE"));
  system_info::SetPicoJsonObjectValue(output, "data", data);

  std::string result = output.serialize();
  const char* result_as_cstr = result.c_str();
  AutoLock lock(&events_list_mutex_);
  for (SystemInfoEventsList::iterator it = local_events_.begin();
       it != local_events_.end(); it++) {
    (*it)->PostMessage(result_as_cstr);
  }
}

bool SysInfoLocale::GetLanguage() {
  char* language_info = vconf_get_str(VCONFKEY_LANGSET);
  if (!language_info)
    return false;

  language_ = std::string(language_info);
  free(language_info);

  return true;
}

bool SysInfoLocale::GetCountry() {
  char* country_info = vconf_get_str(VCONFKEY_REGIONFORMAT);

  if (!country_info)
    return false;

  country_ = std::string(country_info);
  free(country_info);

  return true;
}

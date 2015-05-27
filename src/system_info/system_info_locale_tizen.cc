// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_locale.h"

#include <stdlib.h>

#include "common/picojson.h"

const std::string SysInfoLocale::name_ = "LOCALE";

SysInfoLocale::SysInfoLocale() {}

SysInfoLocale::~SysInfoLocale() {}

void SysInfoLocale::StartListening() {
  vconf_notify_key_changed(VCONFKEY_REGIONFORMAT,
      static_cast<vconf_callback_fn>(OnCountryChanged), this);
  vconf_notify_key_changed(VCONFKEY_LANGSET,
      static_cast<vconf_callback_fn>(OnLanguageChanged), this);
}

void SysInfoLocale::StopListening() {
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

  PostMessageToListeners(output);
}

bool SysInfoLocale::GetLanguage() {
  char* language_info = vconf_get_str(VCONFKEY_LANGSET);
  if (!language_info)
    return false;

  language_ = GetFirstSubstringByDot(std::string(language_info));
  free(language_info);

  return true;
}

bool SysInfoLocale::GetCountry() {
  char* country_info = vconf_get_str(VCONFKEY_REGIONFORMAT);

  if (!country_info)
    return false;

  country_ = GetFirstSubstringByDot(std::string(country_info));
  free(country_info);

  return true;
}

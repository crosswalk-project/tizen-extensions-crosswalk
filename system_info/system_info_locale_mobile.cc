// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_locale.h"

#if defined(TIZEN_MOBILE)
#include <runtime_info.h>
#endif
#include <string>

#include "common/picojson.h"
#include "system_info/system_info_utils.h"

SysInfoLocale::SysInfoLocale(ContextAPI* api)
    : stopping_(false) {
  api_ = api;
}

SysInfoLocale::~SysInfoLocale() {
}

void SysInfoLocale::Get(picojson::value& error,
                       picojson::value& data) {
  // language
  if (!UpdateLanguage()) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get language failed."));
    return;
  }
  system_info::SetPicoJsonObjectValue(data, "language",
      picojson::value(language_));

  // timezone
  if (!UpdateCountry()) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get timezone failed."));
    return;
  }
  system_info::SetPicoJsonObjectValue(data, "country",
      picojson::value(country_));

  system_info::SetPicoJsonObjectValue(error, "message",
      picojson::value(""));
}

bool SysInfoLocale::UpdateLanguage() {
  char* cptr = NULL;
  std::string language_info;

  runtime_info_key_e key = RUNTIME_INFO_KEY_LANGUAGE;
  runtime_info_get_value_string(key, &cptr);

  language_info = cptr;
  if (language_info.empty()) {
    return false;
  } else {
    language_.assign(language_info);
    return true;
  }
}

bool SysInfoLocale::UpdateCountry() {
  char* cptr = NULL;
  std::string country_info;

  runtime_info_key_e key = RUNTIME_INFO_KEY_REGION;
  runtime_info_get_value_string(key, &cptr);

  country_info = cptr;
  if (country_info.empty()) {
    return false;
  } else {
    country_.assign(country_info);
    return true;
  }
}

gboolean SysInfoLocale::OnUpdateTimeout(gpointer user_data) {
  SysInfoLocale* instance = static_cast<SysInfoLocale*>(user_data);

  if (instance->stopping_) {
    instance->stopping_ = false;
    return FALSE;
  }

  std::string oldlanguage_ = instance->language_;
  std::string oldcountry_ = instance->country_;
  instance->UpdateLanguage();
  instance->UpdateCountry();

  if (oldlanguage_ != instance->language_ ||
      oldcountry_ != instance->country_) {
    picojson::value output = picojson::value(picojson::object());
    picojson::value data = picojson::value(picojson::object());

    system_info::SetPicoJsonObjectValue(data, "language",
        picojson::value(instance->language_));
    system_info::SetPicoJsonObjectValue(data, "country",
        picojson::value(instance->country_));
    system_info::SetPicoJsonObjectValue(output, "cmd",
        picojson::value("SystemInfoPropertyValueChanged"));
    system_info::SetPicoJsonObjectValue(output, "prop",
        picojson::value("LOCALE"));
    system_info::SetPicoJsonObjectValue(output, "data", data);

    std::string result = output.serialize();
    instance->api_->PostMessage(result.c_str());
  }

  return TRUE;
}

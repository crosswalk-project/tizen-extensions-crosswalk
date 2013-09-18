// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_LOCALE_H_
#define SYSTEM_INFO_SYSTEM_INFO_LOCALE_H_

#if defined(TIZEN_MOBILE)
#include <vconf.h>
#include <vconf-keys.h>
#endif

#include <glib.h>
#include <string>

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"
#include "system_info/system_info_utils.h"

class SysInfoLocale {
 public:
  static SysInfoLocale& GetSysInfoLocale() {
    static SysInfoLocale instance;
    return instance;
  }
  ~SysInfoLocale() {
    for (SystemInfoEventsList::iterator it = local_events_.begin();
         it != local_events_.end(); it++)
      StopListening(*it);
    pthread_mutex_destroy(&events_list_mutex_);
  }
  void Get(picojson::value& error, picojson::value& data);
  void StartListening(ContextAPI* api);
  void StopListening(ContextAPI* api);

 private:
  explicit SysInfoLocale();
  bool GetLanguage();
  bool GetCountry();

  std::string language_;
  std::string country_;
  pthread_mutex_t events_list_mutex_;

#if defined(GENERIC_DESKTOP)
  static gboolean OnUpdateTimeout(gpointer user_data);

  int timeout_cb_id_;
#elif defined(TIZEN_MOBILE)
  static void OnCountryChanged(keynode_t* node, void* user_data);
  static void OnLanguageChanged(keynode_t* node, void* user_data);
  void Update();
#endif

  DISALLOW_COPY_AND_ASSIGN(SysInfoLocale);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_LOCALE_H_

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_LOCALE_H_
#define SYSTEM_INFO_SYSTEM_INFO_LOCALE_H_

#include <glib.h>
#include <string>

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"
#include "system_info/system_info_utils.h"

class SysInfoLocale {
 public:
  explicit SysInfoLocale(ContextAPI* api);
  ~SysInfoLocale();
  void Get(picojson::value& error, picojson::value& data);
  inline void StartListening() {
    timeout_cb_id_ = g_timeout_add(system_info::default_timeout_interval,
                                   SysInfoLocale::OnUpdateTimeout,
                                   static_cast<gpointer>(this));
  }
  inline void StopListening() {
    if (timeout_cb_id_ > 0)
      g_source_remove(timeout_cb_id_);
}

 private:
  bool UpdateLanguage();
  bool UpdateCountry();
  static gboolean OnUpdateTimeout(gpointer user_data);

  ContextAPI* api_;
  std::string language_;
  std::string country_;
#if defined(GENERIC_DESKTOP)
  bool stopping_;
#endif
  int timeout_cb_id_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoLocale);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_LOCALE_H_

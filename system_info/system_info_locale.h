// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_LOCALE_H_
#define SYSTEM_INFO_SYSTEM_INFO_LOCALE_H_

#include <libudev.h>

#include <string>

#include "common/picojson.h"
#include "common/utils.h"

class SysInfoLocale {
 public:
  static SysInfoLocale& GetSysInfoLocale(picojson::value& error) {
    static SysInfoLocale instance(error);
    return instance;
  }
  ~SysInfoLocale();
  bool GetLocaleInfo(std::string& Language,
                     std::string& Locale);

 private:
  SysInfoLocale(picojson::value& error);

  struct udev* udev_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoLocale);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_LOCALE_H_

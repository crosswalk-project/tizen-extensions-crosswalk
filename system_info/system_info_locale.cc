// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_locale.h"

#include <libudev.h>
#include <locale.h>

#include <fstream>

#include "common/picojson.h"
#include "system_info/system_info_utils.h"

using namespace system_info;

SysInfoLocale::SysInfoLocale(picojson::value& error) {
  udev_ = udev_new();
  if (!udev_) {
    SetPicoJsonObjectValue(error, "message",
        picojson::value("Can't create udev."));
  }
}

SysInfoLocale::~SysInfoLocale() {
  if(udev_)
    udev_unref(udev_);
}

bool SysInfoLocale::GetLocaleInfo(std::string& Language,
                                  std::string& Locale) {
  setlocale(LC_ALL, "");
  const struct lconv * const currentlocale = localeconv();
  std::string info = setlocale(LC_ALL, NULL);
  int pos = info.find('.', 0);
  Language.assign(info, 0, pos);

  std::ifstream in;
  in.open("/etc/timezone");
  getline(in, info);
  in.close();
  pos = info.find('/', 0);
  Locale.assign(info, pos + 1, info.length() - pos - 1);

  return !(Language.empty() || Locale.empty());
}

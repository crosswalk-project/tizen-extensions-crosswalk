// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_SETTING_SYSTEM_SETTING_LOCALE_H_
#define SYSTEM_SETTING_SYSTEM_SETTING_LOCALE_H_

#include <string>

namespace system_setting {

std::string getLocale();
void setLocale(const std::string& locale_str);

}  // namespace system_setting

#endif  // SYSTEM_SETTING_SYSTEM_SETTING_LOCALE_H_

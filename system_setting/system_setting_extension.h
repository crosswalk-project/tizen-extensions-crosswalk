// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_SETTING_SYSTEM_SETTING_EXTENSION_H_
#define SYSTEM_SETTING_SYSTEM_SETTING_EXTENSION_H_

#include "common/extension.h"

class SystemSettingExtension : public common::Extension {
 public:
  SystemSettingExtension();
  virtual ~SystemSettingExtension();

 private:
  // common::Extension implementation.
  virtual common::Instance* CreateInstance();
};

#endif  // SYSTEM_SETTING_SYSTEM_SETTING_EXTENSION_H_

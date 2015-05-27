// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_SETTING_SYSTEM_SETTING_INSTANCE_H_
#define SYSTEM_SETTING_SYSTEM_SETTING_INSTANCE_H_

#include "common/extension.h"

namespace picojson {
class value;
}

class SystemSettingInstance : public common::Instance {
 public:
  SystemSettingInstance();
  virtual ~SystemSettingInstance();

 private:
  enum SystemSettingType {
    HOME_SCREEN = 0,
    LOCK_SCREEN = 1,
    INCOMING_CALL = 2,
    NOTIFICATION_EMAIL = 3,
    LOCALE = 4
  };

  // common::Instance implementation.
  void HandleMessage(const char* message);

  void HandleSetProperty(const picojson::value& msg);
  void HandleGetProperty(const picojson::value& msg);
  void OnPropertyHandled(const char* reply_id, const char* value, int ret);
};

#endif  // SYSTEM_SETTING_SYSTEM_SETTING_INSTANCE_H_

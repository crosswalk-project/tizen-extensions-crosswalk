// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEMSETTING_SYSTEMSETTING_CONTEXT_H_
#define SYSTEMSETTING_SYSTEMSETTING_CONTEXT_H_

#include "common/extension_adapter.h"

namespace picojson {
class value;
}

class SystemSettingContext {
 public:
  SystemSettingContext(ContextAPI* api);
  ~SystemSettingContext();

  // ExtensionAdapter implementation.
  static const char name[];
  static const char* GetJavaScript();
  void HandleMessage(const char* message);
  void HandleSyncMessage(const char* message);

 private:
  enum SystemSettingType {
    HOME_SCREEN = 0,
    LOCK_SCREEN = 1,
    INCOMING_CALL = 2,
    NOTIFICATION_EMAIL = 3,
  };

  void HandleSetProperty(const picojson::value& msg);
  void HandleGetProperty(const picojson::value& msg);
  void OnPropertyHandled(const char* reply_id, const char* value, int ret);

  ContextAPI* api_;
};

#endif  // SYSTEMSETTING_SYSTEMSETTING_CONTEXT_H_

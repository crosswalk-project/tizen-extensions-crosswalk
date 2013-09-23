// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TIME_TIME_CONTEXT_H_
#define TIME_TIME_CONTEXT_H_

#include "common/extension_adapter.h"
#include "common/picojson.h"

class TimeContext {
 public:
  explicit TimeContext(ContextAPI* api)
     : api_(api) {
  }

  // ExtensionAdapter implementation.
  static const char name[];
  static const char* GetJavaScript();
  void HandleMessage(const char*) {}
  void HandleSyncMessage(const char* message);

 private:
  void SetSyncReply(picojson::value v);

  void HandleGetLocalTimeZone(const picojson::value& msg);
  void HandleGetTimeZoneRawOffset(const picojson::value& msg);

  ContextAPI* api_;
};

#endif  // TIME_TIME_CONTEXT_H_

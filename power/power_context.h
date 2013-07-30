// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NOTIFICATION_NOTIFICATION_CONTEXT_H_
#define NOTIFICATION_NOTIFICATION_CONTEXT_H_

#include "common/extension_adapter.h"
#include <map>
#include <string>

namespace picojson {
class value;
}

class PowerContext {
 public:
  PowerContext(ContextAPI* api);
  ~PowerContext();

  void Initialize();

  // ExtensionAdapter implementation.
  static const char name[];
  static const char* GetJavaScript();
  void HandleMessage(const char* message);
  void HandleSyncMessage(const char* message);

 private:
  // These enums must be kept in sync with the JS object notation.
  enum ResourceType {
    SCREEN = 0,
    CPU = 1,
    ResourceTypeValueCount
  };

  enum ResourceState {
    SCREEN_OFF = 0,
    SCREEN_DIM = 1,
    SCREEN_NORMAL = 2,
    SCREEN_BRIGHT = 3,
    CPU_AWAKE = 4,
    ResourceStateValueCount
  };

  void OnScreenStateChanged(ResourceState state);
  void HandleRequest(const picojson::value& msg);
  void HandleRelease(const picojson::value& msg);
  void HandleSetScreenBrightness(const picojson::value& msg);
  void HandleGetScreenBrightness();
  void HandleSetScreenEnabled(const picojson::value& msg);

  ContextAPI* api_;
};

#endif  // NOTIFICATION_NOTIFICATION_CONTEXT_H_

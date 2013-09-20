// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef POWER_POWER_CONTEXT_H_
#define POWER_POWER_CONTEXT_H_

#if defined(GENERIC_DESKTOP)
#include <gio/gio.h>
#else
#include <power.h>
#endif

#include <map>
#include <string>
#include "common/extension_adapter.h"

namespace picojson {
class value;
}

class PowerContext {
 public:
  explicit PowerContext(ContextAPI* api);
  ~PowerContext();

  void PlatformInitialize();
  void PlatformUninitialize();

  // ExtensionAdapter implementation.
  static const char name[];
  static const char* GetJavaScript();
  void HandleMessage(const char* message);
  void HandleSyncMessage(const char* message);

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
#if defined(TIZEN_MOBILE)
  void OnPlatformScreenStateChanged(power_state_e pstate);
#endif

 private:
  void HandleRequest(const picojson::value& msg);
  void HandleRelease(const picojson::value& msg);
  void HandleSetScreenBrightness(const picojson::value& msg);
  void HandleGetScreenBrightness();
  void HandleSetScreenEnabled(const picojson::value& msg);
  void HandleGetScreenState();

  ContextAPI* api_;

#if defined(GENERIC_DESKTOP)
  friend void OnScreenProxyCreatedThunk(GObject* source, GAsyncResult*,
                                        gpointer);
  GDBusProxy* screen_proxy_;
#else
  bool pending_screen_state_change_;
  bool pending_screen_state_reply_;
#endif
};

#endif  // POWER_POWER_CONTEXT_H_

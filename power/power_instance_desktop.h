// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef POWER_POWER_INSTANCE_DESKTOP_H_
#define POWER_POWER_INSTANCE_DESKTOP_H_

#include <gio/gio.h>

#include "common/extension.h"
#include "power/power_types.h"

namespace picojson {
class value;
}

class PowerInstanceDesktop
    : public common::Instance {
 public:
  PowerInstanceDesktop();
  virtual ~PowerInstanceDesktop();

  void OnScreenStateChanged(ResourceState state);

 private:
  // common::Instance implementation.
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* msg);
  void HandleRequest(const picojson::value& msg);
  void HandleRelease(const picojson::value& msg);
  void HandleSetScreenBrightness(const picojson::value& msg);
  void HandleGetScreenBrightness();
  void HandleSetScreenEnabled(const picojson::value& msg);
  void HandleGetScreenState();

  friend void OnScreenProxyCreatedThunk(GObject* source, GAsyncResult*,
                                        gpointer);
  GDBusProxy* screen_proxy_;
};

#endif  // POWER_POWER_INSTANCE_DESKTOP_H_

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef POWER_POWER_INSTANCE_MOBILE_H_
#define POWER_POWER_INSTANCE_MOBILE_H_

#include <power.h>
#include "common/extension.h"
#include "power/power_types.h"

namespace picojson {
class value;
}

class PowerInstanceMobile
    : public common::Instance {
 public:
  PowerInstanceMobile();

  void OnScreenStateChanged(ResourceState state);
  void OnPlatformScreenStateChanged(power_state_e pstate);

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

  bool pending_screen_state_change_;
  bool pending_screen_state_reply_;
};

#endif  // POWER_POWER_INSTANCE_MOBILE_H_

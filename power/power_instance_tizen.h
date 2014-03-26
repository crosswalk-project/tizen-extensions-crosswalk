// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef POWER_POWER_INSTANCE_TIZEN_H_
#define POWER_POWER_INSTANCE_TIZEN_H_

#include <power.h>
#include "common/extension.h"
#include "power/power_types.h"
#include "power/power_event_source_tizen.h"

namespace picojson {
class value;
}

class PowerInstanceMobile
    : public common::Instance, public PowerEventListener {
 public:
  explicit PowerInstanceMobile(PowerEventSource* event_source);
  ~PowerInstanceMobile();

  void DispatchScreenStateChangedToJS(ResourceState state);

 private:
  // PowerEventListener implementation.
  void OnPowerStateChanged(power_state_e state);

  // common::Instance implementation.
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* msg);

  void HandleRequest(const picojson::value& msg);
  void HandleRelease(const picojson::value& msg);
  void HandleSetScreenBrightness(const picojson::value& msg);
  void HandleGetScreenBrightness();
  void HandleSetScreenEnabled(const picojson::value& msg);
  void HandleGetScreenState();
  void HandleSetListenToScreenStateChange(const picojson::value& msg);

  bool js_listening_to_state_change_;
  bool pending_screen_state_change_;
  bool pending_screen_state_reply_;
  PowerEventSource* event_source_;
};

#endif  // POWER_POWER_INSTANCE_TIZEN_H_

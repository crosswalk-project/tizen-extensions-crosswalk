// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "power/power_context.h"
#include "common/picojson.h"

#include <power.h>
#include <pmapi.h>
#include <device.h>
#include <vconf.h>

static void OnPlatformStateChanged(power_state_e state, void *user_data);

void PowerContext::Initialize() {
  // Set initial state.
  power_state_e pstate = power_get_state();
  OnPlatformStateChanged(pstate, this);

  // Hook up for changes.
  power_set_changed_cb(OnPlatformStateChanged, this);
}

ResourceType getResourceType(const picojson::value& msg, bool* error) {
    int type = msg.get("resource").get<int>();
    if (type < 0 || type >= ResourceTypeValueCount)
        error = true;
    return static_cast<ResourceType>(type);
}

ResourceType getResourceState(const picojson::value& msg, bool* error) {
    int type = msg.get("state").get<int>();
    if (type < 0 || type >= ResourceStateValueCount)
        error = true;
    return static_cast<ResourceState>(type);
}

void OnPlatformStateChanged(power_state_e pstate, void *user_data) {
  PowerContext* self = static_cast<PowerContext*>(user_data);

  State state;
  switch (pstate) {
    case POWER_STATE_NORMAL:
      state = SCREEN_NORMAL;
      break;
    case POWER_STATE_SCREEN_DIM:
      state = SCREEN_DIMMED;
      break;
    case POWER_STATE_SCREEN_OFF:
      state = SCREEN_OFF;
      break;
  }

  OnScreenStateChanged(state);
}

void PowerContext::HandleRequest(const picojson::value& msg) {
  bool error = false;
  // ResourceType is unused in this impl, as the JS API verifies
  // that resource type and state fit.
  ResourceState state = getResourceState(msg, &error);
  ASSERT(!error); // Would indicate wrapper error.

  power_state_e pstate;
  switch (state) {
    case SCREEN_NORMAL:
      pstate = POWER_STATE_NORMAL;
      break;
    case SCREEN_DIM:
      pstate = POWER_STATE_SCREEN_OFF;
      break;
    case SCREEN_OFF:
    case CPU_AWAKE:
      pstate = POWER_STATE_SCREEN_OFF;
      break;
  }

  power_lock_state(pstate, 0);
  // FIXME: Check return value and throw exception if needed.
}

void PowerContext::HandleRelease(const picojson::value& msg) {
  bool error = false;
  ResourceType resource = getResourceType(msg, &error);
  ASSERT(!error); // Would indicate wrapper error.

  switch (resource) {
    case SCREEN: {
      power_unlock_state(POWER_STATE_SCREEN_DIM);
      power_unlock_state(POWER_STATE_NORMAL);
      device_set_brightness_from_settings(0);
      power_state_e pstate = power_get_state();
      OnPlatformStateChanged(pstate, this);
      break;
    }
    case CPU: {
      power_unlock_state(POWER_STATE_SCREEN_OFF);
      break;
    }
  }
  // FIXME: Check return value and throw exception if needed.
}

void PowerContext::HandleSetScreenBrightness(const picojson::value& msg) {
  double brightness = msg.get("value").get<double>();

  if (brightness < 0) {
    // Resource brightness.
    device_set_brightness_from_settings(0);
    return;
  }

  int maxBrightness;
  device_get_max_brightness(0, &maxBrightness);

  device_set_brightness(0, static_cast<int>(brightness * maxBrightness));
}

void PowerContext::HandleGetScreenBrightness() {
  int platformBrightness;
  vconf_get_int(VCONFKEY_SETAPPL_LCD_BRIGHTNESS, &platformBrightness);
  double brightness = platformBrightness / 100.0;
  char brightnessAsString[32];
  snprintf(brightnessAsString, 32, "%g", brightness);
  api_->SetSyncReply(brightnessAsString);
}

void PowerContext::HandleSetScreenEnabled(const picojson::value& msg) {
  bool isEnabled = msg.get("value").get<bool>();

  if (isEnabled)
    pm_change_state(LCD_NORMAL);
  else
    pm_change_state(LCD_OFF);

  // FIXME: Check return value and throw Unknown exception if needed.
}

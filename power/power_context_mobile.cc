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

void PowerContext::PlatformInitialize() {
  // Hook up for changes.
  power_set_changed_cb(OnPlatformStateChanged, this);
}

void PowerContext::PlatformUninitialize() {
}

PowerContext::ResourceType getResourceType(const picojson::value& msg, bool* error) {
    int type = msg.get("resource").get<double>();
    if (type < 0 || type >= PowerContext::ResourceTypeValueCount)
        *error = true;
    return static_cast<PowerContext::ResourceType>(type);
}

PowerContext::ResourceState getResourceState(const picojson::value& msg, bool* error) {
    int state = msg.get("state").get<double>();
    if (state < 0 || state >= PowerContext::ResourceStateValueCount)
        *error = true;
    return static_cast<PowerContext::ResourceState>(state);
}

static PowerContext::ResourceState toResourceState(power_state_e pstate)
{
    switch (pstate) {
    case POWER_STATE_NORMAL:
      return PowerContext::SCREEN_NORMAL;
    case POWER_STATE_SCREEN_DIM:
      return PowerContext::SCREEN_DIM;
    case POWER_STATE_SCREEN_OFF:
    default:
      return PowerContext::SCREEN_OFF;
    }
}

void OnPlatformStateChanged(power_state_e pstate, void *data) {
  PowerContext* self = static_cast<PowerContext*>(data);
  self->OnScreenStateChanged(toResourceState(pstate));
}

void PowerContext::HandleRequest(const picojson::value& msg) {
  bool error = false;
  // ResourceType is unused in this impl, as the JS API verifies
  // that resource type and state fit.
  ResourceState state = getResourceState(msg, &error);

  power_state_e pstate;
  switch (state) {
    case PowerContext::SCREEN_NORMAL:
      pstate = POWER_STATE_NORMAL;
      break;
    case PowerContext::SCREEN_DIM:
      pstate = POWER_STATE_SCREEN_DIM;
      break;
    case PowerContext::SCREEN_OFF:
    case PowerContext::CPU_AWAKE:
    default:
      pstate = POWER_STATE_SCREEN_OFF;
      break;
  }

  power_lock_state(pstate, 0);
  // FIXME: Check return value and throw exception if needed.
}

void PowerContext::HandleRelease(const picojson::value& msg) {
  bool error = false;
  ResourceType resource = getResourceType(msg, &error);

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
    default:
      break;
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
  int ret = device_get_max_brightness(0, &maxBrightness);
  if (ret != 0) {
    fprintf(stderr, "Can't get the max brightness from the platform. \n");
    device_set_brightness(0, static_cast<int>(brightness * 1000));
  } else {
    device_set_brightness(0, static_cast<int>(brightness * maxBrightness));
  }
}

void PowerContext::HandleGetScreenState() {
  power_state_e pstate = power_get_state();
  picojson::value::object o;
  o["state"] = picojson::value(static_cast<double>(toResourceState(pstate)));
  picojson::value v(o);
  api_->SetSyncReply(v.serialize().c_str());
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

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "power/power_context.h"

#include <device.h>
#include <glib.h>
#include <pmapi.h>
#include <vconf.h>
#include "common/picojson.h"

void OnPlatformStateChanged(power_state_e state, void *user_data);

void PowerContext::PlatformInitialize() {
  pending_screen_state_change_ = false;
  pending_screen_state_reply_ = false;
  // Hook up for changes.
  power_set_changed_cb(OnPlatformStateChanged, this);
}

void PowerContext::PlatformUninitialize() {
}

PowerContext::ResourceType getResourceType(const picojson::value& msg,
                                           bool* error) {
    int type = msg.get("resource").get<double>();
    if (type < 0 || type >= PowerContext::ResourceTypeValueCount)
        *error = true;
    return static_cast<PowerContext::ResourceType>(type);
}

PowerContext::ResourceState getResourceState(const picojson::value& msg,
                                             bool* error) {
    int state = msg.get("state").get<double>();
    if (state < 0 || state >= PowerContext::ResourceStateValueCount)
        *error = true;
    return static_cast<PowerContext::ResourceState>(state);
}

static PowerContext::ResourceState toResourceState(power_state_e pstate) {
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

void PowerContext::OnPlatformScreenStateChanged(power_state_e pstate) {
  PowerContext::ResourceState state = toResourceState(pstate);
  pending_screen_state_change_ = false;
  OnScreenStateChanged(state);
  if (pending_screen_state_reply_) {
    picojson::value::object o;
    o["state"] = picojson::value(static_cast<double>(state));
    picojson::value v(o);
    pending_screen_state_reply_ = false;
    api_->SetSyncReply(v.serialize().c_str());
  }
}

void OnPlatformStateChanged(power_state_e pstate, void *data) {
  PowerContext* self = static_cast<PowerContext*>(data);
  self->OnPlatformScreenStateChanged(pstate);
}

void PowerContext::HandleRequest(const picojson::value& msg) {
  bool error = false;
  // ResourceType is unused in this impl, as the JS API verifies
  // that resource type and state fit.
  ResourceState state = getResourceState(msg, &error);

  power_state_e pstate;
  int pm_state;
  switch (state) {
    case PowerContext::CPU_AWAKE:
    case PowerContext::SCREEN_NORMAL: {
      pstate = POWER_STATE_NORMAL;
      pm_state = LCD_NORMAL;
      break;
    }
    case PowerContext::SCREEN_DIM: {
      pstate = POWER_STATE_SCREEN_DIM;
      pm_state = LCD_DIM;
      break;
    }
    case PowerContext::SCREEN_BRIGHT: {
      pstate = POWER_STATE_NORMAL;
      pm_state = LCD_NORMAL;
      // FIXME : Set to max brightness when we can call the function properly.
      break;
    }
    case PowerContext::SCREEN_OFF:
    default:
      pstate = POWER_STATE_SCREEN_OFF;
      pm_state = LCD_OFF;
      break;
  }

  pm_change_state(pm_state);
  pm_lock_state(pm_state, GOTO_STATE_NOW, 0);
  int ret = power_lock_state(pstate, 0);
  if (ret != 0) {
    fprintf(stderr, "Can't lock the state of the platform. \n");
    // FIXME: We need to throw an exception here.
    return;
  }
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
  int maxBrightness;
  power_wakeup(false);
  int ret = device_get_max_brightness(0, &maxBrightness);
  if (ret != 0) {
    fprintf(stderr, "Can't get the max brightness from the platform. \n");
    device_set_brightness(0, static_cast<int>(brightness * 100.0));
  } else {
    device_set_brightness(0, static_cast<int>(brightness * maxBrightness));
  }
}

void PowerContext::HandleGetScreenState() {
  if (pending_screen_state_change_) {
    pending_screen_state_reply_ = true;
  } else {
    picojson::value::object o;
    o["state"] = picojson::value(
            static_cast<double>(toResourceState(power_get_state())));
    picojson::value v(o);
    api_->SetSyncReply(v.serialize().c_str());
  }
}

void PowerContext::HandleGetScreenBrightness() {
  int platformBrightness;
  int ret = device_get_brightness(0, &platformBrightness);
  if (ret != 0) {
    fprintf(stderr, "Can't get the brightness from the platform. \n");
    return;
  }

  double brightness = platformBrightness / 100.0;
  char brightnessAsString[32];
  snprintf(brightnessAsString, sizeof(brightnessAsString), "%g", brightness);
  api_->SetSyncReply(brightnessAsString);
}

void PowerContext::HandleSetScreenEnabled(const picojson::value& msg) {
  bool is_enabled = msg.get("value").get<bool>();
  power_state_e current_state = power_get_state();
  if ((is_enabled && current_state != POWER_STATE_SCREEN_OFF)
    || (!is_enabled && current_state == POWER_STATE_SCREEN_OFF))
      return;

  int ret = 0;
  int new_state = is_enabled ? LCD_NORMAL : LCD_OFF;
  ret = pm_change_state(new_state);
  if (ret != 0) {
    // FIXME: Check return value and throw Unknown exception if needed.
    return;
  }
  pending_screen_state_change_ = true;
}

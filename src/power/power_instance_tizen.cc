// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "power/power_instance_tizen.h"

#include <device.h>
#include <glib.h>
#include <pmapi.h>
#include <vconf.h>
#include <string>

#include "common/picojson.h"

PowerInstanceMobile::PowerInstanceMobile(PowerEventSource* event_source)
    : js_listening_to_state_change_(false),
      event_source_(event_source) {
  pending_screen_state_change_ = false;
  pending_screen_state_reply_ = false;

  event_source_->AddListener(this);
}

PowerInstanceMobile::~PowerInstanceMobile() {
  event_source_->RemoveListener(this);
}

ResourceType getResourceType(const picojson::value& msg,
                                           bool* error) {
    int type = msg.get("resource").get<double>();
    if (type < 0 || type >= ResourceTypeValueCount)
        *error = true;
    return static_cast<ResourceType>(type);
}

ResourceState getResourceState(const picojson::value& msg,
                                             bool* error) {
    int state = msg.get("state").get<double>();
    if (state < 0 || state >= ResourceStateValueCount)
        *error = true;
    return static_cast<ResourceState>(state);
}

static ResourceState toResourceState(power_state_e pstate) {
    switch (pstate) {
    case POWER_STATE_NORMAL:
      return SCREEN_NORMAL;
    case POWER_STATE_SCREEN_DIM:
      return SCREEN_DIM;
    case POWER_STATE_SCREEN_OFF:
    default:
      return SCREEN_OFF;
    }
}

void PowerInstanceMobile::HandleMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "PowerRequest") {
    HandleRequest(v);
  } else if (cmd == "PowerRelease") {
    HandleRelease(v);
  } else if (cmd == "PowerSetScreenBrightness") {
    // value of -1 means restore to default value.
    HandleSetScreenBrightness(v);
  } else if (cmd == "PowerSetScreenEnabled") {
    HandleSetScreenEnabled(v);
  } else if (cmd == "SetListenToScreenStateChange") {
    HandleSetListenToScreenStateChange(v);
  } else {
    std::cout << "ASSERT NOT REACHED.\n";
  }
}

void PowerInstanceMobile::HandleSyncMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "PowerGetScreenBrightness") {
    HandleGetScreenBrightness();
  }  else if (cmd == "PowerGetScreenState") {
    HandleGetScreenState();
  } else {
    std::cout << "ASSERT NOT REACHED.\n";
  }
}

void PowerInstanceMobile::DispatchScreenStateChangedToJS(
    ResourceState state) {
  if (!js_listening_to_state_change_)
    return;

  picojson::value::object o;
  o["cmd"] = picojson::value("ScreenStateChanged");
  o["state"] = picojson::value(static_cast<double>(state));

  picojson::value v(o);
  PostMessage(v.serialize().c_str());
}

void PowerInstanceMobile::OnPowerStateChanged(power_state_e pstate) {
  ResourceState state = toResourceState(pstate);
  pending_screen_state_change_ = false;
  DispatchScreenStateChangedToJS(state);

  if (pending_screen_state_reply_) {
    picojson::value::object o;
    o["state"] = picojson::value(static_cast<double>(state));
    picojson::value v(o);
    pending_screen_state_reply_ = false;
    SendSyncReply(v.serialize().c_str());
  }
}

void PowerInstanceMobile::HandleRequest(const picojson::value& msg) {
  bool error = false;
  // ResourceType is unused in this impl, as the JS API verifies
  // that resource type and state fit.
  ResourceState state = getResourceState(msg, &error);

  power_state_e pstate;
  int pm_state;
  switch (state) {
    case CPU_AWAKE:
    case SCREEN_NORMAL: {
      pstate = POWER_STATE_NORMAL;
      pm_state = LCD_NORMAL;
      break;
    }
    case SCREEN_DIM: {
      pstate = POWER_STATE_SCREEN_DIM;
      pm_state = LCD_DIM;
      break;
    }
    case SCREEN_BRIGHT: {
      pstate = POWER_STATE_NORMAL;
      pm_state = LCD_NORMAL;
      // FIXME : Set to max brightness when we can call the function properly.
      break;
    }
    case SCREEN_OFF:
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

void PowerInstanceMobile::HandleRelease(const picojson::value& msg) {
  bool error = false;
  ResourceType resource = getResourceType(msg, &error);

  switch (resource) {
    case SCREEN: {
      power_unlock_state(POWER_STATE_SCREEN_DIM);
      power_unlock_state(POWER_STATE_NORMAL);
      device_set_brightness_from_settings(0);
      power_state_e pstate = power_get_state();
      OnPowerStateChanged(pstate);
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

void PowerInstanceMobile::HandleSetScreenBrightness(
        const picojson::value& msg) {
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

void PowerInstanceMobile::HandleGetScreenState() {
  if (pending_screen_state_change_) {
    pending_screen_state_reply_ = true;
  } else {
    picojson::value::object o;
    o["state"] = picojson::value(
            static_cast<double>(toResourceState(power_get_state())));
    picojson::value v(o);
    SendSyncReply(v.serialize().c_str());
  }
}

void PowerInstanceMobile::HandleGetScreenBrightness() {
  int platformBrightness;
  picojson::value::object o;

  int ret = device_get_brightness(0, &platformBrightness);
  if (ret != 0) {
    fprintf(stderr, "Can't get the brightness from the platform. \n");
    o["error"] = picojson::value("Can't get the brightness from the platform.");
  } else {
    int maxBrightness;
    ret = device_get_max_brightness(0, &maxBrightness);
    if (ret != 0 || maxBrightness <= 0) {
      fprintf(stderr, "Can't get the max brightness from the platform. \n");
      maxBrightness = 100;
    }
    double brightness = platformBrightness / maxBrightness;
    o["brightness"] = picojson::value(brightness);
  }
  picojson::value v(o);
  SendSyncReply(v.serialize().c_str());
}

void PowerInstanceMobile::HandleSetScreenEnabled(const picojson::value& msg) {
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

namespace {

bool GetBoolFromJSONValue(const picojson::value& v, bool* result) {
  if (!result || !v.is<bool>())
    return false;
  *result = v.get<bool>();
  return true;
}

}  // namespace

void PowerInstanceMobile::HandleSetListenToScreenStateChange(
    const picojson::value& msg) {
  bool listening = false;
  if (!GetBoolFromJSONValue(msg.get("value"), &listening))
    return;
  js_listening_to_state_change_ = listening;
}

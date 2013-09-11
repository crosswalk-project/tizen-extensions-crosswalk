// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "power/power_context.h"

#include <iostream>
#include <fstream>
#include "common/picojson.h"

#define DEVICE "/sys/class/backlight/acpi_video0"

static double kMaxBrightness = 0;

static int readInt(const char* pathname) {
  std::ifstream file;
  file.open(pathname, std::ios::in);

  if (!file.is_open())
    return -1;

  char buf[4];
  file.read(buf, 4);
  file.close();

  return atoi(buf);
}

static void writeInt(const char* pathname, int value) {
  std::ofstream file;
  file.open(pathname, std::ios::out);

  if (!file.is_open())
    return;

  char buf[4];
  snprintf(buf, sizeof(buf), "%d", value);

  file.seekp(0, std::ios::beg);
  file.write(buf, 4);
  file.close();
}

void OnScreenProxyCreatedThunk(GObject* source, GAsyncResult* res,
                               gpointer data) {
  PowerContext* context = static_cast<PowerContext*>(data);
  // Returns 0 in case of failure.
  context->screen_proxy_ = g_dbus_proxy_new_for_bus_finish(res, /* error */ 0);
}

void PowerContext::PlatformInitialize() {
  kMaxBrightness = readInt(DEVICE "/max_brightness");

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SESSION,
      G_DBUS_PROXY_FLAGS_NONE,
      NULL, /* GDBusInterfaceInfo */
      "org.gnome.SettingsDaemon",
      "/org/gnome/SettingsDaemon/Power",
      "org.gnome.SettingsDaemon.Power.Screen",
      NULL, /* GCancellable */
      OnScreenProxyCreatedThunk,
      this);
}

void PowerContext::PlatformUninitialize() {
  if (screen_proxy_)
    g_object_unref(screen_proxy_);
}

void PowerContext::HandleRequest(const picojson::value& msg) {
  std::string resource = msg.get("resource").to_str();
  ResourceState state = static_cast<ResourceState>(
      msg.get("state").get<double>());

  switch (state) {
    case SCREEN_OFF:
    case SCREEN_DIM:
    case SCREEN_NORMAL:
    case SCREEN_BRIGHT:
      OnScreenStateChanged(state);
      break;
    default:
      break;
  }
}

void PowerContext::HandleRelease(const picojson::value& msg) {
  std::string resource = msg.get("resource").to_str();
}

void PowerContext::HandleSetScreenBrightness(const picojson::value& msg) {
  double value = msg.get("value").get<double>();

  // Attempt using the GNOME SettingsDaemon service.
  if (screen_proxy_) {
    g_dbus_proxy_call(screen_proxy_,
      "SetPercentage",
      g_variant_new("(u)", static_cast<int>(value * 100)),
      G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, this);
    return;
  }

  // Fallback to manual (requires root).
  if (kMaxBrightness < 1)
    return;

  writeInt(DEVICE "/brightness", value * kMaxBrightness);
}

void PowerContext::HandleGetScreenBrightness() {
  char brightnessAsString[32] = "0.0";

  double value = readInt(DEVICE "/brightness");
  if (kMaxBrightness > 0 && value >= 0)
    snprintf(brightnessAsString, sizeof(brightnessAsString),
             "%g", value / kMaxBrightness);

  api_->SetSyncReply(brightnessAsString);
}

void PowerContext::HandleSetScreenEnabled(const picojson::value& msg) {
  bool isEnabled = msg.get("value").get<bool>();
}

void PowerContext::HandleGetScreenState() {
  picojson::value::object o;
  o["state"] = picojson::value(
      static_cast<double>(PowerContext::SCREEN_NORMAL));
  picojson::value v(o);
  api_->SetSyncReply(v.serialize().c_str());
}

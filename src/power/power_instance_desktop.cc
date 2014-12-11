// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "power/power_instance_desktop.h"

#include <iostream>
#include <fstream>
#include <string>
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
  PowerInstanceDesktop* instance = static_cast<PowerInstanceDesktop*>(data);
  // Returns 0 in case of failure.
  instance->screen_proxy_ = g_dbus_proxy_new_for_bus_finish(res, /* error */ 0);
}

PowerInstanceDesktop::PowerInstanceDesktop() {
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

PowerInstanceDesktop::~PowerInstanceDesktop() {
  if (screen_proxy_)
    g_object_unref(screen_proxy_);
}

void PowerInstanceDesktop::HandleMessage(const char* message) {
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
  } else {
    std::cout << "ASSERT NOT REACHED.\n";
  }
}

void PowerInstanceDesktop::HandleSyncMessage(const char* message) {
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

void PowerInstanceDesktop::OnScreenStateChanged(ResourceState state) {
  picojson::value::object o;
  o["cmd"] = picojson::value("ScreenStateChanged");
  o["state"] = picojson::value(static_cast<double>(state));

  picojson::value v(o);
  PostMessage(v.serialize().c_str());
}

void PowerInstanceDesktop::HandleRequest(const picojson::value& msg) {
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

void PowerInstanceDesktop::HandleRelease(const picojson::value& msg) {
  std::string resource = msg.get("resource").to_str();
}

void PowerInstanceDesktop::HandleSetScreenBrightness(
    const picojson::value& msg) {
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

void PowerInstanceDesktop::HandleGetScreenBrightness() {
  char brightnessAsString[32] = "0.0";

  double value = readInt(DEVICE "/brightness");
  if (kMaxBrightness > 0 && value >= 0)
    snprintf(brightnessAsString, sizeof(brightnessAsString),
             "%g", value / kMaxBrightness);

  SendSyncReply(brightnessAsString);
}

void PowerInstanceDesktop::HandleSetScreenEnabled(const picojson::value& msg) {
  bool isEnabled = msg.get("value").get<bool>();
}

void PowerInstanceDesktop::HandleGetScreenState() {
  picojson::value::object o;
  o["state"] = picojson::value(
      static_cast<double>(SCREEN_NORMAL));
  picojson::value v(o);
  SendSyncReply(v.serialize().c_str());
}

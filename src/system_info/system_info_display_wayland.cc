// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <wayland-client.h>

#include <cstdio>
#include <iostream>

#include "system_info/system_info_display.h"

#include "common/picojson.h"

#if defined(GENERIC_DESKTOP)
  #define ACPI_BACKLIGHT_DIR "/sys/class/backlight/acpi_video0"
#elif defined(TIZEN)
  #define ACPI_BACKLIGHT_DIR "/sys/class/backlight/psb-bl"
#else
  #error "Unsupported platform"
#endif

class Display {
 public:
  Display();

  wl_display* display;
  wl_registry* registry;
  wl_output* output;
  int width;
  int height;
  double physical_width;
  double physical_height;
  int factor;
};

Display::Display()
    : display(NULL),
      registry(NULL),
      output(NULL),
      width(0),
      height(0),
      physical_width(0.0),
      physical_height(0.0),
      factor(1) {}

static void display_handle_geometry(void* data,
                                    wl_output* output,
                                    int x,
                                    int y,
                                    int physical_width,
                                    int physical_height,
                                    int subpixel,
                                    const char* make,
                                    const char* model,
                                    int transform) {
  Display* d = reinterpret_cast<Display*>(data);
  d->physical_width = physical_width;
  d->physical_height = physical_height;
}

static void display_handle_mode(void* data,
                                wl_output* output,
                                uint32_t flags,
                                int width,
                                int height,
                                int refresh) {
  Display* d = reinterpret_cast<Display*>(data);
  // A display has multiple supporting modes.
  // We need to check if it is the current using mode.
  if (flags & WL_OUTPUT_MODE_CURRENT) {
    d->width = width;
    d->height = height;
  }
}

static void display_handle_done(void* data,
                                wl_output* output) {
}

// This method will be called if any screen with UI scale adjustment applied.
// So it is safe to assume factor to be 1.0 (unscaled) by default.
static void display_handle_scale(void* data,
                                 wl_output* output,
                                 int factor) {
  Display* d = reinterpret_cast<Display*>(data);
  d->factor = factor;
}

static void registry_handle_global(void* data,
                                   wl_registry* registry,
                                   uint32_t id,
                                   const char* interface,
                                   uint32_t version) {
  Display* d = reinterpret_cast<Display*>(data);

  static const wl_output_listener kOutputListener = {
    display_handle_geometry,
    display_handle_mode,
    display_handle_done,
    display_handle_scale
  };

  if (strcmp(interface, "wl_output") == 0) {
    void* v = wl_registry_bind(registry, id, &wl_output_interface, 1);
    d->output = reinterpret_cast<wl_output*>(v);
    wl_output_add_listener(d->output, &kOutputListener, d);
  }
}

static void registry_handle_global_remove(void* data,
                                          wl_registry* registry,
                                          uint32_t name) {}

static const wl_registry_listener kRegistryListener = {
    registry_handle_global,
    registry_handle_global_remove
};

static void get_wayland_screen_size(Display* display) {
  display->registry = wl_display_get_registry(display->display);
  wl_registry_add_listener(display->registry, &kRegistryListener, display);
}

const std::string SysInfoDisplay::name_ = "DISPLAY";

SysInfoDisplay::SysInfoDisplay()
    : resolution_width_(0),
      resolution_height_(0),
      dots_per_inch_width_(0),
      dots_per_inch_height_(0),
      physical_width_(0.0),
      physical_height_(0.0),
      brightness_(0.0),
      timeout_cb_id_(0),
      scale_factor_(0) {}

void SysInfoDisplay::Get(picojson::value& error,
                         picojson::value& data) {
  if (!UpdateSize()) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get display size failed."));
    return;
  }

  if (!UpdateBrightness()) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get display brightness failed."));
    return;
  }

  SetData(data);
  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

bool SysInfoDisplay::UpdateSize() {
  Display screen;

  // FIXME(XWALK-1091): Use gfx::Screen Chromium API instead of Wayland API.
  screen.display = wl_display_connect(NULL);

  if (!(screen.display)) {
    std::cerr << "Wayland server connection error" << std::endl;
    return false;
  }

  screen.width = -1;
  get_wayland_screen_size(&screen);

  // Wait for wayland server response using wl_output width return value.
  while (screen.width == -1)
    wl_display_roundtrip(screen.display);

  resolution_width_ = screen.width;
  resolution_height_ = screen.height;
  physical_width_ = screen.physical_width;
  physical_height_ = screen.physical_height;
  scale_factor_ = screen.factor;

  wl_registry_destroy(screen.registry);
  wl_display_flush(screen.display);
  wl_display_disconnect(screen.display);

  return true;
}

bool SysInfoDisplay::UpdateBrightness() {
  char* str_val;

  str_val = system_info::ReadOneLine(ACPI_BACKLIGHT_DIR"/max_brightness");
  if (!str_val) {
    // FIXME(halton): ACPI is not enabled, fallback to maximum.
    brightness_ = 1.0;
    return true;
  }
  double max_val = strtod(str_val, NULL);
  free(str_val);

  str_val = system_info::ReadOneLine(ACPI_BACKLIGHT_DIR"/brightness");
  if (!str_val) {
    // FIXME(halton): ACPI is not enabled, fallback to maximum.
    brightness_ = 1.0;
    return true;
  }
  double val = strtod(str_val, NULL);
  free(str_val);

  brightness_ = val / max_val;
  return true;
}

gboolean SysInfoDisplay::OnUpdateTimeout(gpointer user_data) {
  SysInfoDisplay* instance = static_cast<SysInfoDisplay*>(user_data);

  double old_brightness = instance->brightness_;
  if (!instance->UpdateBrightness()) {
    // Fail to update brightness, wait for next round
    return TRUE;
  }

  int old_resolution_width = instance->resolution_width_;
  int old_resolution_height = instance->resolution_width_;
  double old_physical_width = instance->physical_width_;
  double old_physical_height = instance->physical_height_;
  if (!instance->UpdateSize()) {
    // Fail to update size, wait for next round
    return TRUE;
  }

  if ((old_brightness != instance->brightness_) ||
      (old_resolution_width != instance->resolution_width_) ||
      (old_resolution_height != instance->resolution_width_) ||
      (old_physical_width != instance->physical_width_) ||
      (old_physical_height != instance->physical_height_)) {
    picojson::value output = picojson::value(picojson::object());;
    picojson::value data = picojson::value(picojson::object());

    instance->SetData(data);
    system_info::SetPicoJsonObjectValue(output, "cmd",
        picojson::value("SystemInfoPropertyValueChanged"));
    system_info::SetPicoJsonObjectValue(output, "prop",
        picojson::value("DISPLAY"));
    system_info::SetPicoJsonObjectValue(output, "data", data);

    instance->PostMessageToListeners(output);
  }

  return TRUE;
}

void SysInfoDisplay::SetData(picojson::value& data) {
  system_info::SetPicoJsonObjectValue(data, "brightness",
      picojson::value(brightness_));

  system_info::SetPicoJsonObjectValue(data, "resolutionWidth",
      picojson::value(static_cast<double>(resolution_width_)));
  system_info::SetPicoJsonObjectValue(data, "resolutionHeight",
      picojson::value(static_cast<double>(resolution_height_)));
  system_info::SetPicoJsonObjectValue(data, "physicalWidth",
      picojson::value(physical_width_));
  system_info::SetPicoJsonObjectValue(data, "physicalHeight",
      picojson::value(physical_height_));

#if defined(GENERIC_DESKTOP) || defined(TIZEN_IVI)
  dots_per_inch_width_ = dots_per_inch_height_ = scale_factor_ * 96.0;
#elif defined(TIZEN_MOBILE)
  // A mobile device is considered to be held at 60% of arms length.
  dots_per_inch_width_ = dots_per_inch_height_ = scale_factor * (96.0 / 0.6);
#endif

  system_info::SetPicoJsonObjectValue(data, "dotsPerInchWidth",
      picojson::value(static_cast<double>(dots_per_inch_width_)));
  system_info::SetPicoJsonObjectValue(data, "dotsPerInchHeight",
      picojson::value(static_cast<double>(dots_per_inch_height_)));
}

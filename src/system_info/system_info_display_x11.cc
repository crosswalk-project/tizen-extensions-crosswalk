// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_display.h"

#include <stdio.h>

#include <X11/Xlib.h>

#include "common/picojson.h"

#if defined(GENERIC_DESKTOP)
  #define ACPI_BACKLIGHT_DIR "/sys/class/backlight/acpi_video0"
#elif defined(TIZEN)
  #define ACPI_BACKLIGHT_DIR "/sys/class/backlight/psb-bl"
#else
  #error "Unsupported platform"
#endif

const std::string SysInfoDisplay::name_ = "DISPLAY";

SysInfoDisplay::SysInfoDisplay()
    : resolution_width_(0),
      resolution_height_(0),
      dots_per_inch_width_(0),
      dots_per_inch_height_(0),
      physical_width_(0.0),
      physical_height_(0.0),
      brightness_(0.0),
      timeout_cb_id_(0) {}

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
  Display *dpy = XOpenDisplay(NULL);

  if (NULL == dpy) {
    return false;
  }

  resolution_width_ = DisplayWidth(dpy, DefaultScreen(dpy));
  resolution_height_ = DisplayHeight(dpy, DefaultScreen(dpy));
  physical_width_ = DisplayWidthMM(dpy, DefaultScreen(dpy));
  physical_height_ = DisplayHeightMM(dpy, DefaultScreen(dpy));

  XCloseDisplay(dpy);
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

  // dpi = N * 25.4 pixels / M inch
  dots_per_inch_width_ = physical_width_ == 0 ? 0 :
      static_cast<unsigned long>((resolution_width_ * 25.4) / physical_width_); // NOLINT
  dots_per_inch_height_ = physical_height_ == 0 ? 0 :
      static_cast<unsigned long>((resolution_height_ * 25.4) / physical_height_); // NOLINT

  system_info::SetPicoJsonObjectValue(data, "dotsPerInchWidth",
      picojson::value(static_cast<double>(dots_per_inch_width_)));
  system_info::SetPicoJsonObjectValue(data, "dotsPerInchHeight",
      picojson::value(static_cast<double>(dots_per_inch_height_)));
}

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_display.h"

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include "common/picojson.h"
#include "system_info/system_info_utils.h"

#if defined(GENERIC_DESKTOP)
  #define ACPI_BACKLIGHT_DIR "/sys/class/backlight/acpi_video0"
#elif defined(TIZEN_MOBILE)
  #define ACPI_BACKLIGHT_DIR "/sys/class/backlight/psb-bl"
#else
  #error "Unsupported platform"
#endif

void SysInfoDisplay::Update(picojson::value& error) {
  picojson::object& error_map = error.get<picojson::object>();
  Display *dpy = XOpenDisplay(NULL);

  if (NULL == dpy) {
    error_map["message"] = picojson::value("XOpenDisplay Failed");
    return;
  }

  resolution_width_ = DisplayWidth(dpy, DefaultScreen(dpy));
  if (!resolution_width_) {
    error_map["message"] = picojson::value("SCREEN WIDTH is 0px.");
    XCloseDisplay(dpy);
    return;
  }

  resolution_height_ = DisplayHeight(dpy, DefaultScreen(dpy));
  if (!resolution_height_) {
    error_map["message"] = picojson::value("SCREEN HEIGHT is 0px.");
    XCloseDisplay(dpy);
    return;
  }

  physical_width_ = DisplayWidthMM(dpy, DefaultScreen(dpy));
  if (!physical_width_) {
    error_map["message"] = picojson::value("SCREEN WIDTH is 0mm.");
    XCloseDisplay(dpy);
    return;
  }

  physical_height_ = DisplayHeightMM(dpy, DefaultScreen(dpy));
  if (!physical_width_) {
    error_map["message"] = picojson::value("SCREEN HEIGHT is 0mm.");
    XCloseDisplay(dpy);
    return;
  }

  XCloseDisplay(dpy);
}

SysInfoDisplay::SysInfoDisplay()
    : resolution_width_(0),
      resolution_height_(0),
      physical_width_(0.0),
      physical_height_(0.0) {
}

unsigned long SysInfoDisplay::GetResolutionWidth(picojson::value& error) {
  Update(error);
  return resolution_width_;
}

unsigned long SysInfoDisplay::GetResolutionHeight(picojson::value& error) {
  Update(error);
  return resolution_height_;
}

unsigned long SysInfoDisplay::GetDotsPerInchWidth(picojson::value& error) {
  Update(error);
  // dpi = N * 25.4 pixels / M inch
  return (resolution_width_ * 25.4) / physical_width_;
}

unsigned long SysInfoDisplay::GetDotsPerInchHeight(picojson::value& error) {
  Update(error);
  return (resolution_height_ * 25.4) / physical_height_;
}

double SysInfoDisplay::GetPhysicalWidth(picojson::value& error) {
  Update(error);
  return physical_width_;
}

double SysInfoDisplay::GetPhysicalHeight(picojson::value& error) {
  Update(error);
  return physical_height_;
}

double SysInfoDisplay::GetBrightness(picojson::value& error) {
  char* str_val = NULL;
  char max_path[] = ACPI_BACKLIGHT_DIR"/max_brightness";
  char brightness_path[] = ACPI_BACKLIGHT_DIR"/brightness";
  int max_val, val;

  str_val = system_info::read_one_line(max_path);
  if(NULL == str_val) {
    // FIXME(halton): ACPI is not enabled, fallback to maximum.
    return 1.0;
  }
  max_val = atoi(str_val);
  free(str_val);

  str_val = system_info::read_one_line(brightness_path);
  if(!str_val) {
    // FIXME(halton): ACPI is not enabled, fallback to maximum.
    return 1.0;
  }
  val = atoi(str_val);
  free(str_val);

  return val / max_val;
}

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_DISPLAY_H_
#define SYSTEM_INFO_SYSTEM_INFO_DISPLAY_H_

#include <glib.h>

#include <string>

#include "common/picojson.h"
#include "common/utils.h"
#include "system_info/system_info_instance.h"
#include "system_info/system_info_utils.h"

class SysInfoDisplay : public SysInfoObject {
 public:
  static SysInfoObject& GetInstance() {
    static SysInfoDisplay instance;
    return instance;
  }
  ~SysInfoDisplay() {
    if (timeout_cb_id_ > 0)
      g_source_remove(timeout_cb_id_);
  }
  // Get support
  void Get(picojson::value& error, picojson::value& data);
  // Listerner support
  inline void StartListening() {
    // FIXME(halton): Use Xlib event or D-Bus interface to monitor.
    if (timeout_cb_id_ == 0) {
      timeout_cb_id_ = g_timeout_add(system_info::default_timeout_interval,
                                     SysInfoDisplay::OnUpdateTimeout,
                                     static_cast<gpointer>(this));
    }
  }
  inline void StopListening() {
    if (timeout_cb_id_ > 0) {
      g_source_remove(timeout_cb_id_);
      timeout_cb_id_ = 0;
    }
  }

  static const std::string name_;

 private:
  SysInfoDisplay();

  static gboolean OnUpdateTimeout(gpointer user_data);
  bool UpdateSize();
  bool UpdateBrightness();
  void SetData(picojson::value& data);

  int resolution_width_;
  int resolution_height_;
  unsigned long dots_per_inch_width_; // NOLINT
  unsigned long dots_per_inch_height_; // NOLINT
  double physical_width_;
  double physical_height_;
  double brightness_;
  int timeout_cb_id_;
  int scale_factor_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoDisplay);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_DISPLAY_H_

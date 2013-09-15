// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_DISPLAY_H_
#define SYSTEM_INFO_SYSTEM_INFO_DISPLAY_H_

#include <glib.h>

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"
#include "system_info/system_info_utils.h"

class SysInfoDisplay {
 public:
  explicit SysInfoDisplay(ContextAPI* api);
  ~SysInfoDisplay() {
    if (timeout_cb_id_ > 0)
      g_source_remove(timeout_cb_id_);
}
  // Get support
  void Get(picojson::value& error, picojson::value& data); //NOLINT
  // Listerner support
  inline void StartListening() {
    // FIXME(halton): Use Xlib event or D-Bus interface to monitor.
    timeout_cb_id_ = g_timeout_add(system_info::default_timeout_interval,
                                   SysInfoDisplay::OnUpdateTimeout,
                                   static_cast<gpointer>(this));
  }
  void StopListening() {
    if (timeout_cb_id_ > 0)
      g_source_remove(timeout_cb_id_);
}

 private:
  static gboolean OnUpdateTimeout(gpointer user_data);
  bool UpdateSize();
  bool UpdateBrightness();
  void SetData(picojson::value& data); //NOLINT

  ContextAPI* api_;
  int resolution_width_;
  int resolution_height_;
  double physical_width_;
  double physical_height_;
  double brightness_;
  int timeout_cb_id_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoDisplay);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_DISPLAY_H_

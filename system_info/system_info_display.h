// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_DISPLAY_H_
#define SYSTEM_INFO_SYSTEM_INFO_DISPLAY_H_

#include <glib.h>

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"

class SysInfoDisplay {
 public:
  static SysInfoDisplay& GetSysInfoDisplay(ContextAPI* api) {
    static SysInfoDisplay d(api);
    return d;
  }
  ~SysInfoDisplay() { }
  // Get support
  void Get(picojson::value& error, picojson::value& data);
  // Listerner support
  inline void StartListen() {
    stopping_ = false;
    g_timeout_add_seconds(3,
                          SysInfoDisplay::TimedOutUpdate,
                          static_cast<gpointer>(this));
  }
  void StopListen() { stopping_ = true; }

 private:
  SysInfoDisplay(ContextAPI* api);

  static gboolean TimedOutUpdate(gpointer user_data);
  bool UpdateSize();
  bool UpdateBrightness();
  void SetData(picojson::value& data);

  ContextAPI* api_;
  unsigned long resolution_width_;
  unsigned long resolution_height_;
  double physical_width_;
  double physical_height_;
  double brightness_;
  double stopping_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoDisplay);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_DISPLAY_H_

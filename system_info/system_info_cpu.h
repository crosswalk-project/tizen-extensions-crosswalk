// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_CPU_H_
#define SYSTEM_INFO_SYSTEM_INFO_CPU_H_

#include <glib.h>

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"

class SysInfoCpu {
 public:
  static SysInfoCpu& GetSysInfoCpu(ContextAPI* api) {
    static SysInfoCpu instance(api);
    return instance;
  }
  ~SysInfoCpu() { }
  // Get support
  void Get(picojson::value& error, picojson::value& data);

  // Listerner support
  inline void StartListen() {
    stopping_ = false;
    g_timeout_add_seconds(3,
                          SysInfoCpu::TimedOutUpdate,
                          static_cast<gpointer>(this));
  }
  inline void StopListen() { stopping_ = true; }

 private:
  explicit SysInfoCpu(ContextAPI* api)
      : load_(0.0),
        stopping_(false) {
    api_ = api;
  }

  static gboolean TimedOutUpdate(gpointer user_data);
  bool UpdateLoad();

  ContextAPI* api_;
  double load_;
  double stopping_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoCpu);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_CPU_H_

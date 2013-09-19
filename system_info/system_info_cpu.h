// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_CPU_H_
#define SYSTEM_INFO_SYSTEM_INFO_CPU_H_

#include <glib.h>

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"
#include "system_info/system_info_utils.h"

class SysInfoCpu {
 public:
  explicit SysInfoCpu(ContextAPI* api)
      : load_(0.0),
        old_total_(0),
        old_used_(0),
        timeout_cb_id_(0) {
    api_ = api;
    UpdateLoad();
  }
  ~SysInfoCpu() {
    if (timeout_cb_id_ > 0)
      g_source_remove(timeout_cb_id_);
  }
  // Get support
  void Get(picojson::value& error, picojson::value& data);

  // Listerner support
  inline void StartListening() {
    timeout_cb_id_ = g_timeout_add(system_info::default_timeout_interval,
                                   SysInfoCpu::OnUpdateTimeout,
                                   static_cast<gpointer>(this));
  }
  inline void StopListening() {
    if (timeout_cb_id_ > 0)
      g_source_remove(timeout_cb_id_);
  }

 private:
  static gboolean OnUpdateTimeout(gpointer user_data);
  bool UpdateLoad();

  ContextAPI* api_;
  double load_;
  unsigned long long old_total_; //NOLINT
  unsigned long long old_used_; //NOLINT
  int timeout_cb_id_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoCpu);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_CPU_H_

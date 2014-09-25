// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_CPU_H_
#define SYSTEM_INFO_SYSTEM_INFO_CPU_H_
#include <stdio.h>
#include <glib.h>

#include <string>

#include "common/picojson.h"
#include "common/utils.h"
#include "system_info/system_info_instance.h"
#include "system_info/system_info_utils.h"

class SysInfoCpu : public SysInfoObject {
 public:
  static SysInfoObject& GetInstance() {
    static SysInfoCpu instance;
    return instance;
  }
  ~SysInfoCpu() {
    if (timeout_cb_id_ > 0)
      g_source_remove(timeout_cb_id_);
  }
  // Get support
  void Get(picojson::value& error, picojson::value& data);

  // Listerner support
  void StartListening();
  void StopListening();

  static const std::string name_;

 private:
  SysInfoCpu()
      : load_(0.0),
        old_total_(0),
        old_used_(0),
        timeout_cb_id_(0) {
    UpdateLoad();
  }
  static gboolean OnUpdateTimeout(gpointer user_data);
  bool UpdateLoad();

  double load_;
  unsigned long long old_total_; //NOLINT
  unsigned long long old_used_; //NOLINT
  int timeout_cb_id_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoCpu);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_CPU_H_

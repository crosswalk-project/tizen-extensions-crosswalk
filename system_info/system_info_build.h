// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_BUILD_H_
#define SYSTEM_INFO_SYSTEM_INFO_BUILD_H_

#include <glib.h>
#include <string>

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"
#include "system_info/system_info_utils.h"

class SysInfoBuild {
 public:
  static SysInfoBuild& GetSysInfoBuild() {
    static SysInfoBuild instance;
    return instance;
  }
  ~SysInfoBuild() {
    if (timeout_cb_id_ > 0)
      g_source_remove(timeout_cb_id_);
    pthread_mutex_destroy(&events_list_mutex_);
  }
  void Get(picojson::value& error, picojson::value& data);
  inline void StartListening(ContextAPI* api) {
    AutoLock lock(&events_list_mutex_);
    build_events_.push_back(api);
    if (timeout_cb_id_ == 0) {
      timeout_cb_id_ = g_timeout_add(system_info::default_timeout_interval,
                                     SysInfoBuild::OnUpdateTimeout,
                                     static_cast<gpointer>(this));
    }
  }
  inline void StopListening(ContextAPI* api) {
    AutoLock lock(&events_list_mutex_);
    build_events_.remove(api);
    if (build_events_.empty() && timeout_cb_id_ > 0) {
      g_source_remove(timeout_cb_id_);
      timeout_cb_id_ = 0;
    }
  }

 private:
  explicit SysInfoBuild()
      : timeout_cb_id_(0) {
    pthread_mutex_init(&events_list_mutex_, NULL);
  }

  bool UpdateHardware();
  bool UpdateOSBuild();
  static gboolean OnUpdateTimeout(gpointer user_data);

  pthread_mutex_t events_list_mutex_;
  std::string model_;
  std::string manufacturer_;
  std::string buildversion_;
  int timeout_cb_id_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoBuild);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_BUILD_H_

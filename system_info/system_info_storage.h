// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_STORAGE_H_
#define SYSTEM_INFO_SYSTEM_INFO_STORAGE_H_

#include <glib.h>
#include <libudev.h>
#include <string>

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"
#include "system_info/system_info_utils.h"

class SysInfoStorage {
 public:
  static SysInfoStorage& GetSysInfoStorage(ContextAPI* api) {
    static SysInfoStorage instance(api);
    return instance;
  }
  ~SysInfoStorage();
  // Get support
  void Get(picojson::value& error, picojson::value& data);

  // Listerner support
  inline void StartListening() {
    stopping_ = false;
    // FIXME(halton): Use udev D-Bus interface to monitor.
    g_timeout_add(system_info::default_timeout_interval,
                  SysInfoStorage::OnUpdateTimeout,
                  static_cast<gpointer>(this));
  }
  inline void StopListening() { stopping_ = true; }

 private:
  explicit SysInfoStorage(ContextAPI* api);
  bool Update(picojson::value& error);
  void GetDetails(const std::string& mnt_fsname,
                  const std::string& mnt_dir,
                  picojson::value& error,
                  picojson::value& unit);
  static gboolean OnUpdateTimeout(gpointer user_data);
  std::string GetDevPathFromMountPath(const std::string& mnt_path);

  ContextAPI* api_;
  struct udev* udev_;

  bool stopping_;
  picojson::value units_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoStorage);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_STORAGE_H_

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_STORAGE_H_
#define SYSTEM_INFO_SYSTEM_INFO_STORAGE_H_

#include <glib.h>
#if defined(GENERIC_DESKTOP)
#include <libudev.h>
#endif
#include <string>

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"
#include "system_info/system_info_utils.h"

class SysInfoStorage {
 public:
  static SysInfoStorage& GetSysInfoStorage() {
    static SysInfoStorage instance;
    return instance;
  }
  ~SysInfoStorage();
  void Get(picojson::value& error, picojson::value& data);
  void StartListening(ContextAPI* api);
  void StopListening(ContextAPI* api);

 private:
  explicit SysInfoStorage();
  bool Update(picojson::value& error);
  static gboolean OnUpdateTimeout(gpointer user_data);

  int timeout_cb_id_;
  picojson::value units_;
  pthread_mutex_t events_list_mutex_;

#if defined(GENERIC_DESKTOP)
  void GetDetails(const std::string& mnt_fsname,
                  const std::string& mnt_dir,
                  picojson::value& error,
                  picojson::value& unit);
  std::string GetDevPathFromMountPath(const std::string& mnt_path);

  struct udev* udev_;
#elif defined(TIZEN_MOBILE)
  bool GetInternal(picojson::value& error, picojson::value& unit);
  bool GetMMC(picojson::value& error, picojson::value& unit);
#endif

  DISALLOW_COPY_AND_ASSIGN(SysInfoStorage);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_STORAGE_H_

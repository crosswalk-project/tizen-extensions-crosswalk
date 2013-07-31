// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_STORAGE_H_
#define SYSTEM_INFO_SYSTEM_INFO_STORAGE_H_

#include <libudev.h>

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"

class SysInfoStorage {
 public:
  static SysInfoStorage& GetSysInfoStorage(ContextAPI* api) {
    static SysInfoStorage d(api);
    return d;
  }
  ~SysInfoStorage();
  void Get(picojson::value& error, picojson::value& data);
  void StartListen();
  void StopListen();

 private:
  SysInfoStorage(ContextAPI* api);
  void GetDetails(const std::string& mnt_fsname,
                  const std::string& mnt_dir,
                  picojson::value& error,
                  picojson::value& unit);
  std::string GetDevPathFromMountPath(const std::string& mnt_path);

  ContextAPI* api_;
  struct udev* udev_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoStorage);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_STORAGE_H_

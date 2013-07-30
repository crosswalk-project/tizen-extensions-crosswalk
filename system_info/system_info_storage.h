// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_STORAGE_H_
#define SYSTEM_INFO_SYSTEM_INFO_STORAGE_H_

#include <libudev.h>

#include "common/picojson.h"
#include "common/utils.h"

class SysInfoStorage {
 public:
  static SysInfoStorage& GetSysInfoStorage(picojson::value& error) {
    static SysInfoStorage d(error);
    return d;
  }
  ~SysInfoStorage();
  void Update(picojson::value& error, picojson::value& data);

 private:
  SysInfoStorage(picojson::value& error);

  void GetDetails(const std::string& mnt_fsname,
                  const std::string& mnt_dir,
                  picojson::value& error,
                  picojson::value& unit);
  std::string GetDevPathFromMountPath(const std::string& mnt_path);

  struct udev* udev_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoStorage);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_STORAGE_H_

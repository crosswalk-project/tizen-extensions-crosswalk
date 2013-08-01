// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_BUILD_H_
#define SYSTEM_INFO_SYSTEM_INFO_BUILD_H_

#include <libudev.h>

#include <string>

#include "common/picojson.h"
#include "common/utils.h"

class SysInfoBuild {
 public:
  static SysInfoBuild& GetSysInfoBuild(picojson::value& error) {
    static SysInfoBuild instance(error);
    return instance;
  }
  ~SysInfoBuild();
  bool GetBuildInfo(std::string& Manufactor,
                    std::string& Model,
                    std::string& Build);

 private:
  SysInfoBuild(picojson::value& error);

  struct udev* udev_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoBuild);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_BUILD_H_

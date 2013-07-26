// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_DISPLAY_H_
#define SYSTEM_INFO_SYSTEM_INFO_DISPLAY_H_

#include "common/picojson.h"
#include "common/utils.h"

class SysInfoDisplay {
 public:
  static SysInfoDisplay& GetSysInfoDisplay() {
    static SysInfoDisplay d;
    return d;
  }
  ~SysInfoDisplay() { }

  unsigned long GetResolutionWidth(picojson::value& error);
  unsigned long GetResolutionHeight(picojson::value& error);
  unsigned long GetDotsPerInchWidth(picojson::value& error);
  unsigned long GetDotsPerInchHeight(picojson::value& error);
  double GetPhysicalWidth(picojson::value& error);
  double GetPhysicalHeight(picojson::value& error);
  double GetBrightness(picojson::value& error);

 private:
  SysInfoDisplay();
  void Update(picojson::value& error);

  unsigned long resolution_width_;
  unsigned long resolution_height_;
  double physical_width_;
  double physical_height_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoDisplay);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_DISPLAY_H_

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_SIM_H_
#define SYSTEM_INFO_SYSTEM_INFO_SIM_H_

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"

class SysInfoSim {
 public:
  static SysInfoSim& GetSysInfoSim(
      ContextAPI* api) {
    static SysInfoSim instance(api);
    return instance;
  }
  ~SysInfoSim() { }
  void Get(picojson::value& error, picojson::value& data);
  void StartListening() { }
  void StopListening() { }

 private:
  explicit SysInfoSim(ContextAPI* api) {
    api_ = api;
  }

  ContextAPI* api_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoSim);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_SIM_H_

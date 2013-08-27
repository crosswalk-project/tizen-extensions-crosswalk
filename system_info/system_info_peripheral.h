// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_PERIPHERAL_H_
#define SYSTEM_INFO_SYSTEM_INFO_PERIPHERAL_H_

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"

class SysInfoPeripheral {
 public:
  static SysInfoPeripheral& GetSysInfoPeripheral(
      ContextAPI* api) {
    static SysInfoPeripheral instance(api);
    return instance;
  }
  ~SysInfoPeripheral() { }
  void Get(picojson::value& error, picojson::value& data);
  void StartListening() { }
  void StopListening() { }

 private:
  explicit SysInfoPeripheral(ContextAPI* api) {
    api_ = api;
  }

  ContextAPI* api_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoPeripheral);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_PERIPHERAL_H_

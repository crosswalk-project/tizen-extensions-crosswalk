// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_CELLULAR_NETWORK_H_
#define SYSTEM_INFO_SYSTEM_INFO_CELLULAR_NETWORK_H_

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"

class SysInfoCellularNetwork {
 public:
  static SysInfoCellularNetwork& GetSysInfoCellularNetwork(
      ContextAPI* api) {
    static SysInfoCellularNetwork instance(api);
    return instance;
  }
  ~SysInfoCellularNetwork() { }
  void Get(picojson::value& error, picojson::value& data);
  void StartListen() { }
  void StopListen() { }

 private:
  explicit SysInfoCellularNetwork(ContextAPI* api) {
    api_ = api;
  }

  ContextAPI* api_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoCellularNetwork);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_CELLULAR_NETWORK_H_

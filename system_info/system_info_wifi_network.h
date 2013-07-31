// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_WIFI_NETWORK_H_
#define SYSTEM_INFO_SYSTEM_INFO_WIFI_NETWORK_H_

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"

class SysInfoWifiNetwork {
 public:
  static SysInfoWifiNetwork& GetSysInfoWifiNetwork(
      ContextAPI* api) {
    static SysInfoWifiNetwork instance(api);
    return instance;
  }
  ~SysInfoWifiNetwork() { }
  void Get(picojson::value& error, picojson::value& data);
  void StartListen() { };
  void StopListen() { };

 private:
  SysInfoWifiNetwork(ContextAPI* api) {
    api_ = api;
  }

  ContextAPI* api_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoWifiNetwork);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_WIFI_NETWORK_H_

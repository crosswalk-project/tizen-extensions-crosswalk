// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_NETWORK_H_
#define SYSTEM_INFO_SYSTEM_INFO_NETWORK_H_

#include <string>

#include "common/picojson.h"
#include "common/utils.h"
#include "system_info/system_info_instance.h"
#include "system_info/system_info_utils.h"

enum SystemInfoNetworkType {
  SYSTEM_INFO_NETWORK_NONE = 0,
  SYSTEM_INFO_NETWORK_2G,
  SYSTEM_INFO_NETWORK_2_5G,
  SYSTEM_INFO_NETWORK_3G,
  SYSTEM_INFO_NETWORK_4G,
  SYSTEM_INFO_NETWORK_WIFI,
  SYSTEM_INFO_NETWORK_ETHERNET,
  SYSTEM_INFO_NETWORK_UNKNOWN
};

class SysInfoNetwork {
 public:
  static const std::string name_;

 protected:
  SysInfoNetwork() : type_(SYSTEM_INFO_NETWORK_UNKNOWN) {}
  void SetData(picojson::value& data);
  std::string ToNetworkTypeString(SystemInfoNetworkType type);

  SystemInfoNetworkType type_;

 private:
  DISALLOW_COPY_AND_ASSIGN(SysInfoNetwork);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_NETWORK_H_

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_network.h"

const std::string SysInfoNetwork::name_ = "NETWORK";

std::string
SysInfoNetwork::ToNetworkTypeString(SystemInfoNetworkType type) {
  std::string ret;
  switch (type) {
    case SYSTEM_INFO_NETWORK_NONE:
      ret = "NONE";
      break;
    case SYSTEM_INFO_NETWORK_2G:
      ret = "2G";
      break;
    case SYSTEM_INFO_NETWORK_2_5G:
      ret = "2.5G";
      break;
    case SYSTEM_INFO_NETWORK_3G:
      ret = "3G";
      break;
    case SYSTEM_INFO_NETWORK_4G:
      ret = "4G";
      break;
    case SYSTEM_INFO_NETWORK_WIFI:
      ret = "WIFI";
      break;
    case SYSTEM_INFO_NETWORK_ETHERNET:
      ret = "ETHERNET";
      break;
    case SYSTEM_INFO_NETWORK_UNKNOWN:
    default:
      ret = "UNKNOWN";
  }

  return ret;
}

void SysInfoNetwork::SetData(picojson::value& data) {
  system_info::SetPicoJsonObjectValue(data, "networkType",
      picojson::value(ToNetworkTypeString(type_)));
}

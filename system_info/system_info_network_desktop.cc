// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_network.h"

#include "system_info/system_info_utils.h"

using namespace system_info;

void SysInfoNetwork::Get(picojson::value& error,
                         picojson::value& data) {
  // FIXME(halton): Support other ethernet interface name
  if (IsInterfaceOn("eth0")) {
    SetPicoJsonObjectValue(data, "networkType", picojson::value("ETHERNET"));
    return;
  }

  // FIXME(halton): Support other wireless interface name
  if (IsInterfaceOn("wlan0")) {
    SetPicoJsonObjectValue(data, "networkType", picojson::value("WIFI"));
    return;
  }

  // FIXME(halton): Add other network type support
  SetPicoJsonObjectValue(data, "networkType", picojson::value("NONE"));
}

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_wifi_network.h"

#include "system_info/system_info_utils.h"

using namespace system_info;

void SysInfoWifiNetwork::Get(picojson::value& error,
                             picojson::value& data) {
  // FIXME(halton): Add actual implementation
  SetPicoJsonObjectValue(data, "status", picojson::value("ON"));
  SetPicoJsonObjectValue(data, "ssid", picojson::value("test"));
  SetPicoJsonObjectValue(data, "ipAddress", picojson::value("192.168.11.5"));
  SetPicoJsonObjectValue(data, "ipv6Address",
      picojson::value("fe80::250:56ff:fec0:8"));
  SetPicoJsonObjectValue(data, "signalStrength", picojson::value(0.3));
  SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

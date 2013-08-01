// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_wifi_network.h"

#include "system_info/system_info_utils.h"

void SysInfoWifiNetwork::Get(picojson::value& error,
                             picojson::value& data) {
  // FIXME(halton): Add actual implementation
  system_info::SetPicoJsonObjectValue(data, "status",
      picojson::value("ON"));
  system_info::SetPicoJsonObjectValue(data, "ssid",
      picojson::value("test"));
  system_info::SetPicoJsonObjectValue(data, "ipAddress",
      picojson::value("192.168.11.5"));
  system_info::SetPicoJsonObjectValue(data, "ipv6Address",
      picojson::value("fe80::250:56ff:fec0:8"));
  system_info::SetPicoJsonObjectValue(data, "signalStrength",
      picojson::value(0.3));
  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

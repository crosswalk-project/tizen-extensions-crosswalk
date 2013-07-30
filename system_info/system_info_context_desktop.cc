// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_context.h"

#include "common/picojson.h"
#include "system_info/system_info_utils.h"

using namespace system_info;

void SystemInfoContext::GetDeviceOrientation(picojson::value& error,
                                             picojson::value& data) {
  SetPicoJsonObjectValue(error, "message",
      picojson::value("Device Orientation is not support on desktop."));
}

void SystemInfoContext::GetNetwork(picojson::value& error,
                                   picojson::value& data) {
  // FIXME(halton): Support other ethernet interface name
  if (is_interface_on("eth0")) {
    SetPicoJsonObjectValue(data, "networkType", picojson::value("ETHERNET"));
    return;
  }

  // FIXME(halton): Support other wireless interface name
  if (is_interface_on("wlan0")) {
    SetPicoJsonObjectValue(data, "networkType", picojson::value("WIFI"));
    return;
  }

  // FIXME(halton): Add other network type support
  SetPicoJsonObjectValue(data, "networkType", picojson::value("NONE"));
}

void SystemInfoContext::GetWifiNetwork(picojson::value& error,
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

void SystemInfoContext::GetCellularNetwork(picojson::value& error,
                                           picojson::value& data) {
  SetPicoJsonObjectValue(error, "message",
      picojson::value("Cellular Network is not support on desktop."));
}

void SystemInfoContext::GetSIM(picojson::value& error,
                               picojson::value& data) {
  SetPicoJsonObjectValue(error, "message",
      picojson::value("SIM is not support on desktop."));
}

void SystemInfoContext::GetPeripheral(picojson::value& error,
                                      picojson::value& data) {
  SetPicoJsonObjectValue(error, "message",
      picojson::value("Peripheral is not support on desktop."));
}

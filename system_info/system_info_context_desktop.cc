// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_context.h"

#include "common/picojson.h"
#include "system_info/system_info_utils.h"

void SystemInfoContext::GetDeviceOrientation(picojson::value& error,
                                             picojson::value& data) {
  picojson::object& error_map = error.get<picojson::object>();
  error_map["message"] = picojson::value("Device Orientation is not support on desktop.");
}

void SystemInfoContext::GetNetwork(picojson::value& error,
                                   picojson::value& data) {
  picojson::object& error_map = error.get<picojson::object>();
  picojson::object& data_map = data.get<picojson::object>();

  error_map["message"] = picojson::value("");

  // FIXME(halton): Support other ethernet interface name
  if (system_info::is_interface_on("eth0")) {
    data_map["networkType"] = picojson::value("ETHERNET");
    return;
  }

  // FIXME(halton): Support other wireless interface name
  if (system_info::is_interface_on("wlan0")) {
    data_map["networkType"] = picojson::value("WIFI");
    return;
  }

  // FIXME(halton): Add other network type support
  data_map["networkType"] = picojson::value("NONE");
}

void SystemInfoContext::GetWifiNetwork(picojson::value& error,
                                       picojson::value& data) {
  picojson::object& error_map = error.get<picojson::object>();
  picojson::object& data_map = data.get<picojson::object>();

  // FIXME(halton): Add actual implementation
  data_map["status"] = picojson::value("ON");
  data_map["ssid"] = picojson::value("test");
  data_map["ipAddress"] = picojson::value("192.168.11.5");
  data_map["ipv6Address"] = picojson::value("fe80::250:56ff:fec0:8");
  data_map["signalStrength"] = picojson::value(0.3);
  error_map["message"] = picojson::value("");

  // uncomment out below line to try error
  // error_map["message"] = picojson::value("Get Display failed.");
}

void SystemInfoContext::GetCellularNetwork(picojson::value& error,
                                           picojson::value& data) {
  picojson::object& error_map = error.get<picojson::object>();
  error_map["message"] = picojson::value("Cellular Network is not support on desktop.");
}

void SystemInfoContext::GetSIM(picojson::value& error,
                               picojson::value& data) {
  picojson::object& error_map = error.get<picojson::object>();
  error_map["message"] = picojson::value("SIM is not support on desktop.");
}

void SystemInfoContext::GetPeripheral(picojson::value& error,
                                      picojson::value& data) {
  picojson::object& error_map = error.get<picojson::object>();
  error_map["message"] = picojson::value("Peripheral is not support on desktop.");
}

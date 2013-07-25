// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_context.h"

#include "common/picojson.h"

void SystemInfoContext::GetDeviceOrientation(picojson::value& error,
                                             picojson::value& data) {
  picojson::object& error_map = error.get<picojson::object>();
  error_map["message"] = picojson::value("Device Orientation is not support on desktop.");
}

void SystemInfoContext::GetNetwork(picojson::value& error,
                                   picojson::value& data) {
  picojson::object& error_map = error.get<picojson::object>();
  picojson::object& data_map = data.get<picojson::object>();

  // FIXME(halton): Add actual implementation
  data_map["networkType"] = picojson::value("ETHERNET");
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

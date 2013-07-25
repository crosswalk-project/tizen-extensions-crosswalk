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

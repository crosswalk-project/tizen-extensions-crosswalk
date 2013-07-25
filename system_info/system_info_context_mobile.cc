// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_context.h"

#include "common/picojson.h"

void SystemInfoContext::GetDeviceOrientation(picojson::value& error,
                                             picojson::value& data) {
  picojson::object& error_map = error.get<picojson::object>();
  picojson::object& data_map = data.get<picojson::object>();

  // FIXME(halton): Add actual implementation
  data_map["status"] = picojson::value("PORTRAIT_PRIMARY");
  data_map["isAutoRotation"] = picojson::value(false);
  error_map["message"] = picojson::value("");

  // uncomment out below line to try error
  // error_map["message"] = picojson::value("Get Display failed.");
}

void SystemInfoContext::GetCellularNetwork(picojson::value& error,
                                           picojson::value& data) {
  picojson::object& error_map = error.get<picojson::object>();
  picojson::object& data_map = data.get<picojson::object>();

  // FIXME(halton): Add actual implementation
  data_map["status"] = picojson::value("ON");
  data_map["apn"] = picojson::value("China Mobile");
  data_map["ipAddress"] = picojson::value("192.168.11.5");
  data_map["ipv6Address"] = picojson::value("fe80::250:56ff:fec0:8");
  data_map["mcc"] = picojson::value((double)50);
  data_map["mnc"] = picojson::value((double)51);
  data_map["cellId"] = picojson::value((double)52);
  data_map["lac"] = picojson::value((double)53);
  data_map["isRoaming"] = picojson::value(true);
  data_map["isFlightMode"] = picojson::value(false);
  data_map["imei"] = picojson::value("fake imei - 32324877989");
  error_map["message"] = picojson::value("");

  // uncomment out below line to try error
  // error_map["message"] = picojson::value("Get Display failed.");
}

void SystemInfoContext::GetSIM(picojson::value& error,
                               picojson::value& data) {
  picojson::object& error_map = error.get<picojson::object>();
  picojson::object& data_map = data.get<picojson::object>();

  // FIXME(halton): Add actual implementation
  data_map["state"] = picojson::value("READY");
  data_map["operatorName"] = picojson::value("China Mobile");
  data_map["msisdn"] = picojson::value("12321312");
  data_map["iccid"] = picojson::value("234234234");
  data_map["mcc"] = picojson::value((double)50);
  data_map["mnc"] = picojson::value((double)51);
  data_map["msin"] = picojson::value("China Mobile - msin");
  data_map["spn"] = picojson::value("China Mobile - spn");
  error_map["message"] = picojson::value("");

  // uncomment out below line to try error
  // error_map["message"] = picojson::value("Get Display failed.");
}

void SystemInfoContext::GetPeripheral(picojson::value& error,
                                      picojson::value& data) {
  picojson::object& error_map = error.get<picojson::object>();
  picojson::object& data_map = data.get<picojson::object>();

  // FIXME(halton): Add actual implementation
  data_map["isVideoOutputOn"] = picojson::value(true);
  error_map["message"] = picojson::value("");

  // uncomment out below line to try error
  // error_map["message"] = picojson::value("Get Display failed.");
}

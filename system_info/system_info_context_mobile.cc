// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_context.h"

#include <vconf>

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

void SystemInfoContext::GetNetwork(picojson::value& error,
                                   picojson::value& data) {
  picojson::object& error_map = error.get<picojson::object>();
  picojson::object& data_map = data.get<picojson::object>();

  int service_type = 0;
  if (vconf_get_int(VCONFKEY_TELEPHONY_SVCTYPE, &service_type)) {
    error_map["message"] = picojson::value("Get network type failed.");
    return;
  };

  error_map["message"] = picojson::value("");
  switch (service_type) {
    case VCONFKEY_TELEPHONY_SVCTYPE_NONE:
    case VCONFKEY_TELEPHONY_SVCTYPE_NOSVC:
    case VCONFKEY_TELEPHONY_SVCTYPE_EMERGENCY:
      data_map["networkType"] = picojson::value("NONE");
      break;
    case VCONFKEY_TELEPHONY_SVCTYPE_2G:
      data_map["networkType"] = picojson::value("2G");
      break;
    case VCONFKEY_TELEPHONY_SVCTYPE_2_5G:
    case VCONFKEY_TELEPHONY_SVCTYPE_2_5G_EDGE:
      data_map["networkType"] = picojson::value("2.5G");
      break;
    case VCONFKEY_TELEPHONY_SVCTYPE_3G:
      data_map["networkType"] = picojson::value("3G");
      break;
    case VCONFKEY_TELEPHONY_SVCTYPE_HSDPA:
      NETWORK_TYPE = strdup("4G");
      break;
    default:
      error_map["message"] = picojson::value("UNKNOWN");
  }
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
  picojson::object& data_map = data.get<picojson::object>();

  // FIXME(halton): Add actual implementation
  error_map["message"] = picojson::value("NOT IMPLEMENTED.");
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

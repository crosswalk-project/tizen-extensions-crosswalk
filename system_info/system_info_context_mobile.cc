// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_context.h"

#include <vconf>

#include "common/picojson.h"
#include "system_info/system_info_utils.h"

using namespace system_info;

void SystemInfoContext::GetDeviceOrientation(picojson::value& error,
                                             picojson::value& data) {
  // FIXME(halton): Add actual implementation
  SetPicoJsonObjectValue(data, "status", picojson::value("PORTRAIT_PRIMARY"));
  SetPicoJsonObjectValue(data, "isAutoRotation", picojson::value(false));
  SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

void SystemInfoContext::GetNetwork(picojson::value& error,
                                   picojson::value& data) {
  int service_type = 0;
  if (vconf_get_int(VCONFKEY_TELEPHONY_SVCTYPE, &service_type)) {
    SetPicoJsonObjectValue(error, "message",
        picojson::value("Get network type failed."));
    return;
  };

  SetPicoJsonObjectValue(error, "message", picojson::value(""));
  switch (service_type) {
    case VCONFKEY_TELEPHONY_SVCTYPE_NONE:
    case VCONFKEY_TELEPHONY_SVCTYPE_NOSVC:
    case VCONFKEY_TELEPHONY_SVCTYPE_EMERGENCY:
      SetPicoJsonObjectValue(data, "networkType", picojson::value("NONE"));
      break;
    case VCONFKEY_TELEPHONY_SVCTYPE_2G:
      SetPicoJsonObjectValue(data, "networkType", picojson::value("2G"));
      break;
    case VCONFKEY_TELEPHONY_SVCTYPE_2_5G:
    case VCONFKEY_TELEPHONY_SVCTYPE_2_5G_EDGE:
      SetPicoJsonObjectValue(data, "networkType", picojson::value("2.5G"));
      break;
    case VCONFKEY_TELEPHONY_SVCTYPE_3G:
      SetPicoJsonObjectValue(data, "networkType", picojson::value("3G"));
      break;
    case VCONFKEY_TELEPHONY_SVCTYPE_HSDPA:
      SetPicoJsonObjectValue(data, "networkType", picojson::value("4G"));
      break;
    default:
      SetPicoJsonObjectValue(data, "networkType", picojson::value("UNKNOWN"));
  }
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
  // FIXME(halton): Add actual implementation
  SetPicoJsonObjectValue(error, "message",
      picojson::value("NOT IMPLEMENTED."));
}

void SystemInfoContext::GetSIM(picojson::value& error,
                               picojson::value& data) {
  // FIXME(halton): Add actual implementation
  SetPicoJsonObjectValue(data, "state", picojson::value("READY"));
  SetPicoJsonObjectValue(data, "operatorName", picojson::value("China Mobile"));
  SetPicoJsonObjectValue(data, "msisdn", picojson::value("12321312"));
  SetPicoJsonObjectValue(data, "iccid", picojson::value("234234234"));
  SetPicoJsonObjectValue(data, "mcc", picojson::value(static_cast<double>(50)));
  SetPicoJsonObjectValue(data, "mnc", picojson::value(static_cast<double>(51)));
  SetPicoJsonObjectValue(data, "msin", picojson::value("China Mobile - msin"));
  SetPicoJsonObjectValue(data, "spn", picojson::value("China Mobile - spn"));
  SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

void SystemInfoContext::GetPeripheral(picojson::value& error,
                                      picojson::value& data) {
  // FIXME(halton): Add actual implementation
  SetPicoJsonObjectValue(data, "isVideoOutputOn", picojson::value(true));
  SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

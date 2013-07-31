// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_network.h"

#include <vconf>

#include "system_info/system_info_utils.h"

using namespace system_info;

void SysInfoNetwork::Get(picojson::value& error,
                         picojson::value& data) {
  int service_type = 0;
  if (vconf_get_int(VCONFKEY_TELEPHONY_SVCTYPE, &service_type)) {
    SetPicoJsonObjectValue(error, "message",
        picojson::value("Get network type failed."));
    return;
  }

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

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "system_info/system_info_network_tizen.h"

void SysInfoNetworkTizen::Get(picojson::value& error,
                              picojson::value& data) {
  if (!GetNetworkType()) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get network type faild."));
    return;
  }
  SetData(data);
  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

bool SysInfoNetworkTizen::GetNetworkType() {
  int status = 0;
  if (vconf_get_int(VCONFKEY_NETWORK_STATUS, &status))
    return false;

  switch (status) {
    case VCONFKEY_NETWORK_WIFI:
      type_ = SYSTEM_INFO_NETWORK_WIFI;
      break;
    case VCONFKEY_NETWORK_ETHERNET:
      type_ = SYSTEM_INFO_NETWORK_ETHERNET;
      break;
    case VCONFKEY_NETWORK_BLUETOOTH:
      type_ = SYSTEM_INFO_NETWORK_UNKNOWN;
      break;
    case VCONFKEY_NETWORK_CELLULAR:
      GetCellularNetworkType();
      break;
    default:
      type_ = SYSTEM_INFO_NETWORK_NONE;
  }
  return true;
}

void SysInfoNetworkTizen::OnNetworkTypeChanged(keynode_t* node,
                                               void* user_data) {
  SysInfoNetworkTizen* network = static_cast<SysInfoNetworkTizen*>(user_data);
  network->GetNetworkType();
  network->SendUpdate();
}

void SysInfoNetworkTizen::SendUpdate() {
  picojson::value output = picojson::value(picojson::object());
  picojson::value data = picojson::value(picojson::object());

  SetData(data);
  system_info::SetPicoJsonObjectValue(output, "cmd",
      picojson::value("SystemInfoPropertyValueChanged"));
  system_info::SetPicoJsonObjectValue(output, "prop",
      picojson::value("NETWORK"));
  system_info::SetPicoJsonObjectValue(output, "data", data);

  PostMessageToListeners(output);
}

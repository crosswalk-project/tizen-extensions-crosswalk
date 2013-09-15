// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_wifi_network.h"

#include "system_info/system_info_utils.h"

SysInfoWifiNetwork::SysInfoWifiNetwork(ContextAPI* api)
    : status_("OFF"),
      ssid_(""),
      ip_address_(""),
      ipv6_address_(""),
      signal_strength_(0.0) {
  api_ = api;
  PlatformInitialize();
}

void SysInfoWifiNetwork::Get(picojson::value& error, //NOLINT
                             picojson::value& data) {
  if (!Update(error)) {
    if (error.get("message").to_str().empty())
      system_info::SetPicoJsonObjectValue(error, "message",
          picojson::value("Get wifi network info faild."));
    return;
  }

  SetData(data);
  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

void SysInfoWifiNetwork::SendUpdate() {
  picojson::value output = picojson::value(picojson::object());
  picojson::value data = picojson::value(picojson::object());

  SetData(data);
  system_info::SetPicoJsonObjectValue(output, "cmd",
      picojson::value("SystemInfoPropertyValueChanged"));
  system_info::SetPicoJsonObjectValue(output, "prop",
      picojson::value("WIFI_NETWORK"));
  system_info::SetPicoJsonObjectValue(output, "data", data);

  std::string result = output.serialize();
  api_->PostMessage(result.c_str());
}

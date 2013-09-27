// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_wifi_network.h"

void SysInfoWifiNetwork::Get(picojson::value& error,
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
  const char* result_as_cstr = result.c_str();
  AutoLock lock(&events_list_mutex_);
  for (SystemInfoEventsList::iterator it = wifi_network_events_.begin();
    it != wifi_network_events_.end(); it++) {
    (*it)->PostMessage(result_as_cstr);
  }
}

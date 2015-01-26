// Copyright (c) 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_cellular_network.h"

const std::string SysInfoCellularNetwork::name_ = "CELLULAR_NETWORK";

void SysInfoCellularNetwork::SetData(picojson::value& data) {
  system_info::SetPicoJsonObjectValue(data, "status",
      picojson::value(status_));
  system_info::SetPicoJsonObjectValue(data, "apn",
      picojson::value(apn_));
  system_info::SetPicoJsonObjectValue(data, "ipAddress",
      picojson::value(ipAddress_));
  system_info::SetPicoJsonObjectValue(data, "ipv6Address",
     picojson::value(ipv6Address_));
  system_info::SetPicoJsonObjectValue(data, "mcc",
      picojson::value(static_cast<double>(mcc_)));
  system_info::SetPicoJsonObjectValue(data, "mnc",
      picojson::value(static_cast<double>(mnc_)));
  system_info::SetPicoJsonObjectValue(data, "cellId",
      picojson::value(static_cast<double>(cellId_)));
  system_info::SetPicoJsonObjectValue(data, "lac",
      picojson::value(static_cast<double>(lac_)));
  system_info::SetPicoJsonObjectValue(data, "isRoaming",
      picojson::value(isRoaming_));
  system_info::SetPicoJsonObjectValue(data, "isFlightMode",
      picojson::value(isFlightMode_));
  system_info::SetPicoJsonObjectValue(data, "imei",
      picojson::value(imei_));
}

void SysInfoCellularNetwork::SendUpdate() {
  picojson::value output = picojson::value(picojson::object());
  picojson::value data = picojson::value(picojson::object());

  SetData(data);
  system_info::SetPicoJsonObjectValue(output, "cmd",
      picojson::value("SystemInfoPropertyValueChanged"));
  system_info::SetPicoJsonObjectValue(output, "prop",
      picojson::value("CELLULAR_NETWORK"));
  system_info::SetPicoJsonObjectValue(output, "data", data);

  PostMessageToListeners(output);
}

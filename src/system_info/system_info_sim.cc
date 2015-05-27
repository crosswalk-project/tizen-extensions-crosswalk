// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_sim.h"

const std::string SysInfoSim::name_ = "SIM";

void SysInfoSim::SetJsonValues(picojson::value& data) {
  system_info::SetPicoJsonObjectValue(data, "state",
      picojson::value(ToSimStateString(state_)));
  system_info::SetPicoJsonObjectValue(data, "operatorName",
      picojson::value(operator_name_));
  system_info::SetPicoJsonObjectValue(data, "msisdn",
      picojson::value(msisdn_));
  system_info::SetPicoJsonObjectValue(data, "iccid",
      picojson::value(iccid_));
  system_info::SetPicoJsonObjectValue(data, "mcc",
      picojson::value(static_cast<double>(mcc_)));
  system_info::SetPicoJsonObjectValue(data, "mnc",
      picojson::value(static_cast<double>(mnc_)));
  system_info::SetPicoJsonObjectValue(data, "msin",
      picojson::value(msin_));
  system_info::SetPicoJsonObjectValue(data, "spn",
      picojson::value(spn_));
}

std::string SysInfoSim::ToSimStateString(SystemInfoSimState state) {
  switch (state) {
    case SYSTEM_INFO_SIM_ABSENT:
      return "ABSENT";
    case SYSTEM_INFO_SIM_INITIALIZING:
      return "INITIALIZING";
    case SYSTEM_INFO_SIM_READY:
      return "READY";
    case SYSTEM_INFO_SIM_PIN_REQUIRED:
      return "PIN_REQUIRED";
    case SYSTEM_INFO_SIM_PUB_REQUIRED:
      return "PUB_REQUIRED";
    case SYSTEM_INFO_SIM_NETWORK_LOCKED:
      return "NETWORK_LOCKED";
    case SYSTEM_INFO_SIM_SIM_LOCKED:
      return "SIM_LOCKED";
    case SYSTEM_INFO_SIM_UNKNOWN:
    default:
      return "UNKNOWN";
  }
}

// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_sim.h"

const std::string SysInfoSim::name_ = "SIM";

SysInfoSim::SysInfoSim()
    : state_(SYSTEM_INFO_SIM_UNKNOWN),
      operator_name_(""),
      msisdn_(""),
      iccid_(""),
      mcc_(0),
      mnc_(0),
      msin_(""),
      spn_("") {}

SysInfoSim::~SysInfoSim() {}

void SysInfoSim::Get(picojson::value& error,
                     picojson::value& data) {
  GetSimProperties();
  GetOperatorNameAndSpn();
  SetJsonValues(data);
  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

void SysInfoSim::UpdateSimProperty(const gchar* key, GVariant* var_val) {}

void SysInfoSim::OnSimPropertyChanged(GDBusConnection* conn,
                                      const gchar* sender_name,
                                      const gchar* object_path,
                                      const gchar* iface,
                                      const gchar* signal_name,
                                      GVariant* parameters,
                                      gpointer data) {}


void SysInfoSim::GetSimProperties() {}
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
void SysInfoSim::GetOperatorNameAndSpn() {}

void SysInfoSim::StartListening() {}

void SysInfoSim::StopListening() {}

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

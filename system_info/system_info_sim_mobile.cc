// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_sim.h"

#include "system_info/system_info_utils.h"

void SysInfoSim::Get(picojson::value& error, //NOLINT
                     picojson::value& data) {
  char* s = NULL;

  if (sim_get_icc_id(&s) != SIM_ERROR_NONE) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get iccid failed."));
    return;
  }
  if (s) {
    iccid_ = s;
    free(s);
    system_info::SetPicoJsonObjectValue(data, "iccid",
        picojson::value(iccid_));
  } else {
    system_info::SetPicoJsonObjectValue(data, "iccid",
        picojson::value(""));
  }

  s = NULL;
  if (sim_get_mcc(&s) != SIM_ERROR_NONE) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get mcc failed."));
    return;
  }
  if (s) {
    mcc_ = s;
    free(s);
    system_info::SetPicoJsonObjectValue(data, "mcc",
        picojson::value(mcc_));
  } else {
    system_info::SetPicoJsonObjectValue(data, "mcc",
        picojson::value(""));
  }

  s = NULL;
  if (sim_get_mnc(&s) != SIM_ERROR_NONE) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get mnc failed."));
    return;
  }
  if (s) {
    mnc_ = s;
    free(s);
    system_info::SetPicoJsonObjectValue(data, "mnc",
        picojson::value(mcc_));
  } else {
    system_info::SetPicoJsonObjectValue(data, "mnc",
        picojson::value(""));
  }

  s = NULL;
  if (sim_get_msin(&s) != SIM_ERROR_NONE) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get msin failed."));
    return;
  }
  if (s) {
    msin_ = s;
    free(s);
    system_info::SetPicoJsonObjectValue(data, "msin",
        picojson::value(msin_));
  } else {
    system_info::SetPicoJsonObjectValue(data, "msin",
        picojson::value(""));
  }

  s = NULL;
  if (sim_get_spn(&s) != SIM_ERROR_NONE) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get spn failed."));
    return;
  }
  if (s) {
    spn_ = s;
    free(s);
    system_info::SetPicoJsonObjectValue(data, "spn",
        picojson::value(spn_));
  } else {
    system_info::SetPicoJsonObjectValue(data, "spn",
        picojson::value(""));
  }

  s = NULL;
  char* short_name = NULL;
  if (sim_get_cphs_operator_name(&s, &short_name) != SIM_ERROR_NONE) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get operator name failed."));
    return;
  }
  if (s && short_name) {
    operatorName_ = s;
    free(s);
    free(short_name);
    system_info::SetPicoJsonObjectValue(data, "operatorName",
        picojson::value(operatorName_));
  } else {
    system_info::SetPicoJsonObjectValue(data, "operatorName",
        picojson::value(""));
  }

  sim_state_e state = SIM_STATE_UNKNOWN;
  if (sim_get_state(&state) != SIM_ERROR_NONE) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get state failed."));
    return;
  }
  state_ = Get_systeminfo_sim_state(state);
  system_info::SetPicoJsonObjectValue(data, "state",
      picojson::value(ToSimStateString(state_)));

  s = NULL;
  if (sim_get_subscriber_number(&s) != SIM_ERROR_NONE) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get SIM card subscriber number failed."));
    return;
  }
  if (s) {
    msisdn_ = s;
    free(s);
    system_info::SetPicoJsonObjectValue(data, "msisdn",
        picojson::value(msisdn_));
  } else {
    system_info::SetPicoJsonObjectValue(data, "msisdn",
        picojson::value(""));
  }

  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

std::string SysInfoSim::ToSimStateString(SystemInfoSimState state) {
  std::string ret;
  switch (state) {
    case SYSTEM_INFO_SIM_ABSENT:
      ret = "ABSENT";
      break;
    case SYSTEM_INFO_SIM_INITIALIZING:
      ret = "INITIALIZING";
      break;
    case SYSTEM_INFO_SIM_READY:
      ret = "READY";
      break;
    case SYSTEM_INFO_SIM_PIN_REQUIRED:
      ret = "PIN_REQUIRED";
      break;
    case SYSTEM_INFO_SIM_PUB_REQUIRED:
      ret = "PUB_REQUIRED";
      break;
    case SYSTEM_INFO_SIM_NETWORK_LOCKED:
      ret = "NETWORK_LOCKED";
      break;
    case SYSTEM_INFO_SIM_SIM_LOCKED:
      ret = "SIM_LOCKED";
      break;
    case SYSTEM_INFO_SIM_UNKNOWN:
    default:
      ret = "UNKNOWN";
  }
  return ret;
}

void SysInfoSim::StartListening() {
  sim_set_state_changed_cb(OnSimStateChanged, this);
  isRegister_ = true;
}

void SysInfoSim::StopListening() {
  sim_unset_state_changed_cb();
  isRegister_ = false;
}

SysInfoSim::SystemInfoSimState SysInfoSim::Get_systeminfo_sim_state
    (sim_state_e state) {
  SystemInfoSimState sstate;
  // FIXME(jiajia): missed some standand states
  switch (state) {
    case SIM_STATE_UNAVAILABLE:
      sstate = SYSTEM_INFO_SIM_ABSENT;
      break;
    case SIM_STATE_UNKNOWN:
      sstate = SYSTEM_INFO_SIM_UNKNOWN;
      break;
    case SIM_STATE_LOCKED:
      sstate = SYSTEM_INFO_SIM_NETWORK_LOCKED;
      break;
    case SIM_STATE_AVAILABLE:
      sstate = SYSTEM_INFO_SIM_READY;
      break;
    default:
      sstate = SYSTEM_INFO_SIM_UNKNOWN;
  }
  return sstate;
}

void SysInfoSim::OnSimStateChanged(sim_state_e state, void *user_data) {
  SysInfoSim* sim = static_cast<SysInfoSim*>(user_data);
  picojson::value output = picojson::value(picojson::object());;
  picojson::value data = picojson::value(picojson::object());

  SystemInfoSimState sstate = sim->Get_systeminfo_sim_state(state);
  system_info::SetPicoJsonObjectValue(data, "state",
      picojson::value(sim->ToSimStateString(sstate)));

  system_info::SetPicoJsonObjectValue(output, "cmd",
      picojson::value("SystemInfoPropertyValueChanged"));
  system_info::SetPicoJsonObjectValue(output, "prop",
      picojson::value("SIM"));
  system_info::SetPicoJsonObjectValue(output, "data", data);
  std::string result = output.serialize();
  sim->api_->PostMessage(result.c_str());
}

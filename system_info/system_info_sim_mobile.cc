// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_sim.h"

void SysInfoSim::Get(picojson::value& error,
                     picojson::value& data) {
  if (!QuerySIMStatus() ||
      !QuerySIM(sim_get_cphs_operator_name, operatorName_) ||
      !QuerySIM(sim_get_subscriber_number, msisdn_) ||
      !QuerySIM(sim_get_icc_id, iccid_) ||
      !QuerySIM(sim_get_mcc, mcc_) ||
      !QuerySIM(sim_get_mnc, mnc_) ||
      !QuerySIM(sim_get_msin, msin_) ||
      !QuerySIM(sim_get_spn, spn_)) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get SIM data failed."));
    return;
  }

  SetJsonValues(data);
  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

bool SysInfoSim::QuerySIMStatus() {
  sim_state_e state = SIM_STATE_UNKNOWN;
  if (sim_get_state(&state) == SIM_ERROR_NONE) {
    state_ = GetSystemInfoSIMState(state);
    return true;
  }
  return false;
}

bool SysInfoSim::QuerySIM(SIMGetterFunction1 getter,
                          std::string& member,
                          const std::string& default_value) {
  char* value = NULL;
  switch (getter(&value)) {
    case SIM_ERROR_NONE:
      member = value ? std::string(value) : default_value;
      free(value);
      return true;
    case SIM_ERROR_NOT_AVAILABLE:
      member = default_value;
      return true;
  }
  return false;
}

bool SysInfoSim::QuerySIM(SIMGetterFunction2 getter,
                          std::string& member,
                          const std::string& default_value) {
  char* value1 = NULL;
  char* value2 = NULL;
  switch (getter(&value1, &value2)) {
    case SIM_ERROR_NONE:
      if (value1)
        member = std::string(value1);
      else if (value2)
        member = std::string(value2);
      else
        member = default_value;
      free(value1);
      free(value2);
      return true;
    case SIM_ERROR_NOT_AVAILABLE:
      member = default_value;
      return true;
  }
  return false;
}

bool SysInfoSim::QuerySIM(SIMGetterFunction1 getter,
                          unsigned int& member,
                          const unsigned int& default_value) {
  char* value = NULL;
  switch (getter(&value)) {
    case SIM_ERROR_NONE:
      member = value ? strtoul(value, NULL, 0) : default_value;
      if (errno == ERANGE) {
        fprintf(stderr, "QuerySIM strtoul error: out of range");
        member = default_value;
      }
      free(value);
      return true;
    case SIM_ERROR_NOT_AVAILABLE:
      member = default_value;
      return true;
  }
  return false;
}

void SysInfoSim::SetJsonValues(picojson::value& data) {
  system_info::SetPicoJsonObjectValue(data, "state",
      picojson::value(ToSimStateString(state_)));
  system_info::SetPicoJsonObjectValue(data, "operatorName",
      picojson::value(operatorName_));
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

void SysInfoSim::StartListening(ContextAPI* api) {
  AutoLock lock(&events_list_mutex_);
  sim_events_.push_back(api);
  if (sim_events_.size() == 1)
    sim_set_state_changed_cb(OnSimStateChanged, this);
}

void SysInfoSim::StopListening(ContextAPI* api) {
  AutoLock lock(&events_list_mutex_);
  sim_events_.remove(api);
  if (sim_events_.empty())
    sim_unset_state_changed_cb();
}

SysInfoSim::SystemInfoSimState SysInfoSim::GetSystemInfoSIMState
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

  sim->state_ = sim->GetSystemInfoSIMState(state);
  if (!sim->QuerySIM(sim_get_cphs_operator_name, sim->operatorName_) ||
      !sim->QuerySIM(sim_get_subscriber_number, sim->msisdn_) ||
      !sim->QuerySIM(sim_get_icc_id, sim->iccid_) ||
      !sim->QuerySIM(sim_get_mcc, sim->mcc_) ||
      !sim->QuerySIM(sim_get_mnc, sim->mnc_) ||
      !sim->QuerySIM(sim_get_msin, sim->msin_) ||
      !sim->QuerySIM(sim_get_spn, sim->spn_))
    return;
  sim->SetJsonValues(data);

  system_info::SetPicoJsonObjectValue(output, "cmd",
      picojson::value("SystemInfoPropertyValueChanged"));
  system_info::SetPicoJsonObjectValue(output, "prop",
      picojson::value("SIM"));
  system_info::SetPicoJsonObjectValue(output, "data", data);
  std::string result = output.serialize();
  const char* result_as_cstr = result.c_str();
  AutoLock lock(&(sim->events_list_mutex_));
  for (SystemInfoEventsList::iterator it = sim_events_.begin();
       it != sim_events_.end(); it++)
    (*it)->PostMessage(result_as_cstr);
}

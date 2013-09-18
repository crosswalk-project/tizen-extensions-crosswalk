// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_cellular_network.h"

#include <net_connection.h>
#include <system_info.h>

void SysInfoCellularNetwork::SetCellStatus() {
  int cell_status = 0;

  if (vconf_get_bool(VCONFKEY_3G_ENABLE, &cell_status) != 0) {
    status_ = "Unknown";
    return;
  }

  status_ = cell_status == 1 ? "ON" : "OFF";
}

void SysInfoCellularNetwork::SetAPN() {
  connection_h connectionHandle = NULL;
  if (connection_create(&connectionHandle) != CONNECTION_ERROR_NONE) {
    return;
  }

  int ret_val;
  connection_profile_h profileHandle = NULL;
  if (status_ == "ON") {
    ret_val = connection_get_current_profile(connectionHandle, &profileHandle);
  } else {
    ret_val = connection_get_default_cellular_service_profile(connectionHandle,
        CONNECTION_CELLULAR_SERVICE_TYPE_INTERNET, &profileHandle);
  }

  if (ret_val != CONNECTION_ERROR_NONE)
    return;

  char* apn = NULL;
  if (connection_profile_get_cellular_apn(profileHandle, &apn) !=
      CONNECTION_ERROR_NONE)
    return;

  apn_ = std::string(apn);
  free(apn);

  connection_profile_destroy(profileHandle);
  connection_destroy(connectionHandle);
}

void SysInfoCellularNetwork::SetIpAddress() {
  char* address = vconf_get_str(VCONFKEY_NETWORK_IP);
  if (address == NULL) {
    ipAddress_ = "";
    return;
  }

  ipAddress_ = address;
  free(address);
}

void SysInfoCellularNetwork::SetMCC() {
  int plmn_int = 0;

  if (vconf_get_int(VCONFKEY_TELEPHONY_PLMN, &plmn_int) != 0) {
    mcc_ = 0;
    return;
  }

  mcc_ = plmn_int / 100;
}

void SysInfoCellularNetwork::SetMNC() {
  int plmn_int = 0;
  if (vconf_get_int(VCONFKEY_TELEPHONY_PLMN, &plmn_int) != 0) {
    mnc_ = 0;
    return;
  }

  mnc_ = plmn_int % 100;
}

void SysInfoCellularNetwork::SetCellId() {
  if (vconf_get_int(VCONFKEY_TELEPHONY_CELLID, &cellId_) != 0)
    cellId_ = -1;
}

void SysInfoCellularNetwork::SetLAC() {
  if (vconf_get_int(VCONFKEY_TELEPHONY_LAC, &lac_) != 0)
    lac_ = -1;
}

void SysInfoCellularNetwork::SetIsRoaming() {
  int roaming_state = 0;
  if (vconf_get_int(VCONFKEY_TELEPHONY_SVC_ROAM, &roaming_state) != 0) {
    isRoaming_ = false;
    return;
  }

  isRoaming_ = (roaming_state == 1);
}

void SysInfoCellularNetwork::SetFlightMode() {
  int is_flight_mode = 0;
  if (vconf_get_bool(VCONFKEY_TELEPHONY_FLIGHT_MODE, &is_flight_mode) != 0) {
    isFlightMode_ = false;
    return;
  }

  isFlightMode_ = (is_flight_mode == 1);
}

void SysInfoCellularNetwork::SetIMEI() {
  char* imei = NULL;
  if (system_info_get_value_string(SYSTEM_INFO_KEY_MOBILE_DEVICE_ID, &imei) !=
      SYSTEM_INFO_ERROR_NONE) {
    imei_ = "";
    return;
  }

  imei_ = imei;
  free(imei);
}

void SysInfoCellularNetwork::Get(picojson::value& error,
                                 picojson::value& data) {
  SetCellStatus();
  SetAPN();
  SetIpAddress();
  SetMCC();
  SetMNC();
  SetCellId();
  SetLAC();
  SetIsRoaming();
  SetFlightMode();
  SetIMEI();
  SetData(data);
  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

void SysInfoCellularNetwork::SetData(picojson::value& data) {
  system_info::SetPicoJsonObjectValue(data, "status",
        picojson::value(status_));
  system_info::SetPicoJsonObjectValue(data, "apn",
        picojson::value(apn_));
  system_info::SetPicoJsonObjectValue(data, "ipAddress",
        picojson::value(ipAddress_));
  // FIXME(jiajia): not supported
  system_info::SetPicoJsonObjectValue(data, "ipv6Address",
        picojson::value("NOT SUPPORTTED"));
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

  std::string result = output.serialize();
  const char* result_as_cstr = result.c_str();
  AutoLock lock(&events_list_mutex_);
  for (SystemInfoEventsList::iterator it = cellular_events_.begin();
       it != cellular_events_.end(); it++) {
    (*it)->PostMessage(result_as_cstr);
  }
}

void SysInfoCellularNetwork::UpdateCellStatus(int status) {
  std::string new_status;
  if (status == 1) {
    new_status = "ON";
  } else if (status == 0) {
    new_status = "OFF";
  } else {
    new_status = "Unknown";
  }
  if (status_ == new_status)
    return;

  status_ = new_status;
  SetAPN();
  SetIpAddress();
  SetIsRoaming();
  SendUpdate();
}

void SysInfoCellularNetwork::UpdateIpAddress(char* ip) {
  if (!ip || strcmp(ipAddress_.c_str(), ip) == 0) {
    return;
  }

  ipAddress_ = ip;
  SendUpdate();
}

void SysInfoCellularNetwork::UpdateCellId(int cell_id) {
  if (cellId_ == cell_id)
    return;

  cellId_ = cell_id;
  SetCellStatus();
  SetAPN();
  SetIpAddress();
  SetMCC();
  SetMNC();
  SetLAC();
  SetIsRoaming();
  SetFlightMode();
  SendUpdate();
}

void SysInfoCellularNetwork::UpdateLAC(int lac) {
  if (lac_ == lac)
    return;

  lac_ = lac;
  SendUpdate();
}

void SysInfoCellularNetwork::UpdateRoamingState(int is_roaming) {
  if (isRoaming_ == is_roaming)
    return;

  isRoaming_ = (is_roaming == 1);
  SendUpdate();
}

void SysInfoCellularNetwork::UpdateFlightMode(int flight_mode) {
  if (isFlightMode_ == flight_mode)
    return;

  isFlightMode_ = (flight_mode == 1);
  SetCellStatus();
  SetAPN();
  SetIpAddress();
  SetMCC();
  SetMNC();
  SetLAC();
  SendUpdate();
}

void SysInfoCellularNetwork::OnCellStatusChanged(keynode_t* node,
                                                 void* user_data) {
  int status = vconf_keynode_get_bool(node);

  SysInfoCellularNetwork* cellular =
      static_cast<SysInfoCellularNetwork*>(user_data);

  cellular->UpdateCellStatus(status);
}

void SysInfoCellularNetwork::OnIpChanged(keynode_t* node, void* user_data) {
  char* ip = vconf_keynode_get_str(node);
  SysInfoCellularNetwork* cellular =
      static_cast<SysInfoCellularNetwork*>(user_data);

  cellular->UpdateIpAddress(ip);
}

void SysInfoCellularNetwork::OnCellIdChanged(keynode_t* node, void* user_data) {
  int cell_id = vconf_keynode_get_int(node);
  SysInfoCellularNetwork* cellular =
      static_cast<SysInfoCellularNetwork*>(user_data);

  cellular->UpdateCellId(cell_id);
}

void SysInfoCellularNetwork::OnLocationAreaCodeChanged(keynode_t* node,
                                                       void* user_data) {
  int lac = vconf_keynode_get_int(node);
  SysInfoCellularNetwork* cellular =
      static_cast<SysInfoCellularNetwork*>(user_data);

  cellular->UpdateLAC(lac);
}

void SysInfoCellularNetwork::OnRoamingStateChanged(keynode_t* node,
                                                   void* user_data) {
  int is_roaming = vconf_keynode_get_bool(node);
  SysInfoCellularNetwork* cellular =
      static_cast<SysInfoCellularNetwork*>(user_data);

  cellular->UpdateRoamingState(is_roaming);
}

void SysInfoCellularNetwork::OnFlightModeChanged(keynode_t* node,
                                                 void* user_data) {
  int flight_mode = vconf_keynode_get_bool(node);
  SysInfoCellularNetwork* cellular =
      static_cast<SysInfoCellularNetwork*>(user_data);

  cellular->UpdateFlightMode(flight_mode);
}

void SysInfoCellularNetwork::StartListening(ContextAPI* api) {
  AutoLock lock(&events_list_mutex_);
  cellular_events_.push_back(api);

  if (cellular_events_.size() > 1)
    return;

  vconf_notify_key_changed(VCONFKEY_3G_ENABLE,
      (vconf_callback_fn)OnCellStatusChanged, this);
  vconf_notify_key_changed(VCONFKEY_NETWORK_IP,
      (vconf_callback_fn)OnIpChanged, this);
  vconf_notify_key_changed(VCONFKEY_TELEPHONY_CELLID,
      (vconf_callback_fn)OnCellIdChanged, this);
  vconf_notify_key_changed(VCONFKEY_TELEPHONY_LAC,
      (vconf_callback_fn)OnLocationAreaCodeChanged, this);
  vconf_notify_key_changed(VCONFKEY_TELEPHONY_SVC_ROAM,
      (vconf_callback_fn)OnRoamingStateChanged, this);
  vconf_notify_key_changed(VCONFKEY_TELEPHONY_FLIGHT_MODE,
      (vconf_callback_fn)OnFlightModeChanged, this);
}

void SysInfoCellularNetwork::StopListening(ContextAPI* api) {
  AutoLock lock(&events_list_mutex_);
  cellular_events_.remove(api);

  if (!cellular_events_.empty())
    return;

  vconf_ignore_key_changed(VCONFKEY_3G_ENABLE,
      (vconf_callback_fn)OnCellStatusChanged);
  vconf_ignore_key_changed(VCONFKEY_NETWORK_IP,
      (vconf_callback_fn)OnIpChanged);
  vconf_ignore_key_changed(VCONFKEY_TELEPHONY_CELLID,
      (vconf_callback_fn)OnCellIdChanged);
  vconf_ignore_key_changed(VCONFKEY_TELEPHONY_LAC,
      (vconf_callback_fn)OnLocationAreaCodeChanged);
  vconf_ignore_key_changed(VCONFKEY_TELEPHONY_SVC_ROAM,
      (vconf_callback_fn)OnRoamingStateChanged);
  vconf_ignore_key_changed(VCONFKEY_TELEPHONY_FLIGHT_MODE,
      (vconf_callback_fn)OnFlightModeChanged);
}

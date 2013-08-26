// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_cellular_network.h"

#include <system_info.h>

#include "system_info/system_info_utils.h"

void SysInfoCellularNetwork::SetCellStatus() {
  int cell_status = 0;

  if (vconf_get_bool(VCONFKEY_3G_ENABLE, &cell_status) != 0) {
    status_ = "Unknown";
    return;
  }

  status_ = cell_status == 1 ? "ON" : "OFF";
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

void SysInfoCellularNetwork::SetMcc() {
  int plmn_int = 0;

  if (vconf_get_int(VCONFKEY_TELEPHONY_PLMN, &plmn_int) != 0) {
    mcc_ = "";
    return;
  }

  std::stringstream ss;
  ss << plmn_int;
  std::string s = ss.str();
  if (s.size() < 3)
    mcc_ = s;
  else
    mcc_.assign(s, 0, 3);
}

void SysInfoCellularNetwork::SetMnc() {
  int plmn_int = 0;
  if (vconf_get_int(VCONFKEY_TELEPHONY_PLMN, &plmn_int) != 0) {
    mnc_ = "";
    return;
  }

  std::stringstream ss;
  ss << plmn_int;
  std::string s = ss.str();
  if (s.size() < 4)
    mnc_ = "0";
  else
    mnc_.assign(s, 3, 3);
}

void SysInfoCellularNetwork::SetCellId() {
  if (vconf_get_int(VCONFKEY_TELEPHONY_CELLID, &cellId_) != 0)
    cellId_ = -1;
}

void SysInfoCellularNetwork::SetLac() {
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

void SysInfoCellularNetwork::SetImei() {
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
  SetIpAddress();
  SetMcc();
  SetMnc();
  SetCellId();
  SetLac();
  SetIsRoaming();
  SetFlightMode();
  SetImei();
  SetData(data);
  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

void SysInfoCellularNetwork::SetData(picojson::value& data) {
  system_info::SetPicoJsonObjectValue(data, "status",
        picojson::value(status_));
  // FIXME(jiajia): find whick key reflects this prop
  system_info::SetPicoJsonObjectValue(data, "apn",
        picojson::value("Unknown"));
  system_info::SetPicoJsonObjectValue(data, "ipAddress",
        picojson::value(ipAddress_));
  // FIXME(jiajia): not supported
  system_info::SetPicoJsonObjectValue(data, "ipv6Address",
        picojson::value("NOT SUPPORTTED"));
  system_info::SetPicoJsonObjectValue(data, "mcc",
        picojson::value(mcc_));
  system_info::SetPicoJsonObjectValue(data, "mnc",
        picojson::value(mnc_));
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
  api_->PostMessage(result.c_str());
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
  SetIpAddress();
  SetMcc();
  SetMnc();
  SetLac();
  SetIsRoaming();
  SetFlightMode();
  SendUpdate();
}

void SysInfoCellularNetwork::UpdateLacChanged(int lac) {
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
  SetIpAddress();
  SetMcc();
  SetMnc();
  SetLac();  
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

void SysInfoCellularNetwork::OnLacChanged(keynode_t* node, void* user_data) {
  int lac = vconf_keynode_get_int(node);
  SysInfoCellularNetwork* cellular =
      static_cast<SysInfoCellularNetwork*>(user_data);

  cellular->UpdateLacChanged(lac);
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

void SysInfoCellularNetwork::PlatformInitialize() {
  vconf_notify_key_changed(VCONFKEY_3G_ENABLE,
      (vconf_callback_fn)OnCellStatusChanged, this);

  vconf_notify_key_changed(VCONFKEY_NETWORK_IP,
      (vconf_callback_fn)OnIpChanged, this);

  vconf_notify_key_changed(VCONFKEY_TELEPHONY_CELLID,
      (vconf_callback_fn)OnCellIdChanged, this);

  vconf_notify_key_changed(VCONFKEY_TELEPHONY_LAC,
      (vconf_callback_fn)OnCellIdChanged, this);

  vconf_notify_key_changed(VCONFKEY_TELEPHONY_SVC_ROAM,
      (vconf_callback_fn)OnRoamingStateChanged, this);

  vconf_notify_key_changed(VCONFKEY_TELEPHONY_FLIGHT_MODE,
      (vconf_callback_fn)OnFlightModeChanged, this);
}

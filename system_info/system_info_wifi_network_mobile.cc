// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_wifi_network.h"

#include <net_connection.h>

#include "system_info/system_info_utils.h"

const char* WIFI_STATE = "memory/wifi/state";

SysInfoWifiNetwork::~SysInfoWifiNetwork() {
}

void SysInfoWifiNetwork::PlatformInitialize() {
  stopping_ = false;
}

bool SysInfoWifiNetwork::Update(picojson::value& error) {
  connection_h connect = NULL;
  if (connection_create(&connect) != CONNECTION_ERROR_NONE) {
    if (error.get("message").to_str().empty())
      system_info::SetPicoJsonObjectValue(error, "message",
          picojson::value("Get wifi connection faild."));
    return false;
  }

  connection_wifi_state_e state;
  if (connection_get_wifi_state(connect, &state) != CONNECTION_ERROR_NONE) {
    if (error.get("message").to_str().empty())
      system_info::SetPicoJsonObjectValue(error, "message",
          picojson::value("Get wifi state faild."));
    return false;
  }

  connection_profile_h profile = NULL;
  if (connection_get_current_profile(connect, &profile) !=
      CONNECTION_ERROR_NONE) {
    if (error.get("message").to_str().empty())
      system_info::SetPicoJsonObjectValue(error, "message",
          picojson::value("Get wifi connection profile faild."));
    return false;
  }

  char* ipv4 = NULL;
  if (connection_profile_get_ip_address(profile,
      CONNECTION_ADDRESS_FAMILY_IPV4, &ipv4) !=
      CONNECTION_ERROR_NONE) {
    if (error.get("message").to_str().empty())
      system_info::SetPicoJsonObjectValue(error, "message",
          picojson::value("Get wifi ipv4 address faild."));
    return false;
  }

  char* essid = NULL;
  if (connection_profile_get_wifi_essid(profile, &essid) !=
      CONNECTION_ERROR_NONE) {
    if (error.get("message").to_str().empty())
      system_info::SetPicoJsonObjectValue(error, "message",
          picojson::value("Get wifi essid faild."));
    return false;
  }

  int rssi = 0;
  if (connection_profile_get_wifi_rssi(profile, &rssi) !=
      CONNECTION_ERROR_NONE) {
    if (error.get("message").to_str().empty())
      system_info::SetPicoJsonObjectValue(error, "message",
          picojson::value("Get wifi rssi faild."));
    return false;
  }


  if (state == CONNECTION_WIFI_STATE_CONNECTED) {
    status_ = "ON";
  } else {
    status_ = "OFF";
    ip_address_ = "";
    ipv6_address_ = "";
    ssid_ = "";
    signal_strength_ = 0.0;
    return true;
  }

  // FIXME(guanxian): get ipv6 address not supported at connection.
  char* ipv6 = NULL;
  if (connection_profile_get_ip_address(profile,
      CONNECTION_ADDRESS_FAMILY_IPV6, &ipv6) !=
      CONNECTION_ERROR_NONE) {
    ipv6_address_ = "";
  }

  ip_address_.assign(ipv4);
  ssid_.assign(essid);
  signal_strength_ = rssi / 100.0;

  return true;
}

void SysInfoWifiNetwork::SetData(picojson::value& data) {
  system_info::SetPicoJsonObjectValue(data, "status",
      picojson::value(status_));
  system_info::SetPicoJsonObjectValue(data, "ssid",
      picojson::value(ssid_));
  system_info::SetPicoJsonObjectValue(data, "ipAddress",
      picojson::value(ip_address_));
  system_info::SetPicoJsonObjectValue(data, "ipv6Address",
      picojson::value(ipv6_address_));
  system_info::SetPicoJsonObjectValue(data, "signalStrength",
      picojson::value(signal_strength_));
}

gboolean SysInfoWifiNetwork::OnUpdateTimeout(gpointer user_data) {
  SysInfoWifiNetwork* instance = static_cast<SysInfoWifiNetwork*>(user_data);
  if (instance->stopping_) {
    instance->stopping_ = false;
    return FALSE;
  }

  double signal_strength = instance->signal_strength_;
  std::string status = instance->status_;
  std::string ssid = instance->ssid_;
  std::string ip_address = instance->ip_address_;
  std::string ipv6_address = instance->ipv6_address_;
  picojson::value error = picojson::value(picojson::object());
  if (!instance->Update(error)) {
    // Failed to update, wait for next round.
    return TRUE;
  }

  if ((status != instance->status_) ||
      (ssid != instance->ssid_) ||
      (ip_address != instance->ip_address_) ||
      (ipv6_address != instance->ipv6_address_) ||
      (signal_strength != instance->signal_strength_)) {
    instance->SendUpdate();
  }

  return TRUE;
}

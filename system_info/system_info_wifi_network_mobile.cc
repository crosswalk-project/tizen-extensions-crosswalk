// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_wifi_network.h"

namespace {

const double kWifiSignalStrengthDivisor = 100.0;

}  // namespace

SysInfoWifiNetwork::SysInfoWifiNetwork()
    : signal_strength_(0.0),
      ip_address_(""),
      ipv6_address_(""),
      ssid_(""),
      status_("OFF"),
      connection_handle_(NULL),
      connection_profile_handle_(NULL) {
  PlatformInitialize();
}

void SysInfoWifiNetwork::PlatformInitialize() {
  if (connection_create(&connection_handle_) != CONNECTION_ERROR_NONE) {
    connection_handle_ = NULL;
    connection_profile_handle_ = NULL;
  } else {
    if (connection_get_current_profile(connection_handle_,
        &connection_profile_handle_) !=
        CONNECTION_ERROR_NONE)
    connection_profile_handle_ = NULL;
  }
}

SysInfoWifiNetwork::~SysInfoWifiNetwork() {
  if (connection_profile_handle_)
    free(connection_profile_handle_);
  if (connection_handle_)
    free(connection_handle_);
}

void SysInfoWifiNetwork::StartListening() {
  if (connection_handle_) {
    connection_set_type_changed_cb(connection_handle_,
                                   OnTypeChanged, this);
    connection_set_ip_address_changed_cb(connection_handle_,
                                         OnIPChanged, this);
  }
}

void SysInfoWifiNetwork::StopListening() {
  if (connection_handle_) {
    connection_unset_type_changed_cb(connection_handle_);
    connection_unset_ip_address_changed_cb(connection_handle_);
  }
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

bool SysInfoWifiNetwork::Update(picojson::value& error) {
  if (!connection_handle_) {
    if (error.get("message").to_str().empty())
      system_info::SetPicoJsonObjectValue(error, "message",
          picojson::value("Get wifi connection faild."));
    return false;
  }

  if (!GetType()) {
    status_ = "OFF";
    ssid_ = "";
    ip_address_ = "";
    ipv6_address_ = "";
    signal_strength_ = 0.0;
    return true;
  }

  if (!GetIPv4Address())
    ip_address_ = "";

  if (!GetIPv6Address())
    ipv6_address_ = "";

  if (!GetSSID())
    ssid_ = "";

  if (!GetSignalStrength())
    signal_strength_ = 0.0;

  return true;
}

bool SysInfoWifiNetwork::GetType() {
  connection_type_e type_state;
  if (connection_get_type(connection_handle_, &type_state) !=
      CONNECTION_ERROR_NONE)
    return false;

  if (type_state != CONNECTION_TYPE_WIFI)
    return false;

  if (connection_get_current_profile(connection_handle_,
      &connection_profile_handle_) !=
      CONNECTION_ERROR_NONE) {
    connection_profile_handle_ = NULL;
    return false;
  }

  status_ = "ON";
  return true;
}

bool SysInfoWifiNetwork::GetIPv4Address() {
  char* ipv4 = NULL;
  if (connection_profile_get_ip_address(connection_profile_handle_,
      CONNECTION_ADDRESS_FAMILY_IPV4, &ipv4) !=
      CONNECTION_ERROR_NONE) {
    free(ipv4);
    return false;
  }

  ip_address_ = std::string(ipv4);
  free(ipv4);
  return true;
}

bool SysInfoWifiNetwork::GetIPv6Address() {
  char* ipv6 = NULL;
  if (connection_profile_get_ip_address(connection_profile_handle_,
      CONNECTION_ADDRESS_FAMILY_IPV6, &ipv6) !=
      CONNECTION_ERROR_NONE) {
    free(ipv6);
    return false;
  }

  // FIXME(guanxian): get ipv6 address not supported at connection.
  ipv6_address_ = "";
  free(ipv6);
  return true;
}

bool SysInfoWifiNetwork::GetSSID() {
  char* essid = NULL;
  if (connection_profile_get_wifi_essid(connection_profile_handle_, &essid) !=
      CONNECTION_ERROR_NONE) {
    free(essid);
    return false;
  }

  ssid_ = std::string(essid);
  free(essid);
  return true;
}

bool SysInfoWifiNetwork::GetSignalStrength() {
  int rssi = 0;
  if (connection_profile_get_wifi_rssi(connection_profile_handle_, &rssi) !=
      CONNECTION_ERROR_NONE)
    return false;

  signal_strength_ = static_cast<double>(rssi) / kWifiSignalStrengthDivisor;
  return true;
}

void SysInfoWifiNetwork::OnIPChanged(const char* ipv4_addr,
                                     const char* ipv6_addr, void* user_data) {
  SysInfoWifiNetwork* wifi = static_cast<SysInfoWifiNetwork*>(user_data);
  if (!wifi->GetType()) {
    wifi->status_ = "OFF";
    wifi->ip_address_ = "";
    wifi->ipv6_address_ = "";
    wifi->ssid_ = "";
    wifi->signal_strength_ = 0.0;
  } else {
    std::string ipv4_address(ipv4_addr);
    if (ipv4_address != wifi->ip_address_)
      wifi->ip_address_ = ipv4_address;

    std::string ipv6_address(ipv6_addr);
    if (ipv6_address != wifi->ipv6_address_)
      wifi->ipv6_address_ = ipv6_address;

    if (!wifi->GetSSID())
      wifi->ssid_ = "";

    if (!wifi->GetSignalStrength())
      wifi->signal_strength_ = 0.0;
  }

  wifi->SendUpdate();
}

void SysInfoWifiNetwork::OnTypeChanged(connection_type_e type,
                                       void* user_data) {
  SysInfoWifiNetwork* wifi = static_cast<SysInfoWifiNetwork*>(user_data);
  if (type != CONNECTION_TYPE_WIFI) {
    wifi->status_ = "OFF";
    wifi->ip_address_ = "";
    wifi->ipv6_address_ = "";
    wifi->ssid_ = "";
    wifi->signal_strength_ = 0.0;
  } else {
    if (!wifi->GetType())
      wifi->status_ = "OFF";

    if (!wifi->GetIPv4Address())
      wifi->ip_address_ = "";

    if (!wifi->GetIPv6Address())
      wifi->ipv6_address_ = "";

    if (!wifi->GetSSID())
      wifi->ssid_ = "";

    if (!wifi->GetSignalStrength())
      wifi->signal_strength_ = 0.0;
  }

  wifi->SendUpdate();
}

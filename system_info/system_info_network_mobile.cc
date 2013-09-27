// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_network.h"

#include <vconf.h>

SysInfoNetwork::SysInfoNetwork()
    : type_(SYSTEM_INFO_NETWORK_UNKNOWN),
      connection_handle_(NULL) {
  pthread_mutex_init(&events_list_mutex_, NULL);
  PlatformInitialize();
}

void SysInfoNetwork::PlatformInitialize() {
  if (connection_create(&connection_handle_) != CONNECTION_ERROR_NONE)
    connection_handle_ = NULL;
}

SysInfoNetwork::~SysInfoNetwork() {
  for (SystemInfoEventsList::iterator it = network_events_.begin();
       it != network_events_.end(); it++) {
    StopListening(*it);
  }
  if (connection_handle_)
    free(connection_handle_);
  pthread_mutex_destroy(&events_list_mutex_);
}

void SysInfoNetwork::StartListening(ContextAPI* api) {
  AutoLock lock(&events_list_mutex_);
  network_events_.push_back(api);
  if (connection_handle_ && network_events_.size() == 1) {
    connection_set_type_changed_cb(connection_handle_,
                                   OnTypeChanged, this);
  }
}

void SysInfoNetwork::StopListening(ContextAPI* api) {
  AutoLock lock(&events_list_mutex_);
  network_events_.remove(api);
  if (network_events_.empty() && connection_handle_) {
    connection_unset_type_changed_cb(connection_handle_);
  }
}

bool SysInfoNetwork::Update(picojson::value& error) {
  if (!connection_handle_) {
    if (error.get("message").to_str().empty())
      system_info::SetPicoJsonObjectValue(error, "message",
          picojson::value("Get connection faild."));
    return false;
  }

  connection_type_e connection_type;
  if (connection_get_type(connection_handle_, &connection_type) !=
      CONNECTION_ERROR_NONE) {
    if (error.get("message").to_str().empty())
      system_info::SetPicoJsonObjectValue(error, "message",
          picojson::value("Get net state faild."));
    return false;
  }

  if (connection_type == CONNECTION_TYPE_WIFI) {
    type_ = SYSTEM_INFO_NETWORK_WIFI;
    return true;
  }

  if (!GetNetworkType()) {
    if (error.get("message").to_str().empty())
      system_info::SetPicoJsonObjectValue(error, "message",
          picojson::value("Get network type at vconf faild."));
    return false;
  }

  return true;
}

bool SysInfoNetwork::GetNetworkType() {
  int service_type = 0;
  if (vconf_get_int(VCONFKEY_TELEPHONY_SVCTYPE, &service_type))
    return false;

  switch (service_type) {
    case VCONFKEY_TELEPHONY_SVCTYPE_NONE:
    case VCONFKEY_TELEPHONY_SVCTYPE_NOSVC:
    case VCONFKEY_TELEPHONY_SVCTYPE_EMERGENCY:
      type_ = SYSTEM_INFO_NETWORK_NONE;
      break;
    case VCONFKEY_TELEPHONY_SVCTYPE_2G:
      type_ = SYSTEM_INFO_NETWORK_2G;
      break;
    case VCONFKEY_TELEPHONY_SVCTYPE_2_5G:
    case VCONFKEY_TELEPHONY_SVCTYPE_2_5G_EDGE:
      type_ = SYSTEM_INFO_NETWORK_2_5G;
      break;
    case VCONFKEY_TELEPHONY_SVCTYPE_3G:
      type_ = SYSTEM_INFO_NETWORK_3G;
      break;
    case VCONFKEY_TELEPHONY_SVCTYPE_HSDPA:
      type_ = SYSTEM_INFO_NETWORK_4G;
      break;
    default:
      type_ = SYSTEM_INFO_NETWORK_UNKNOWN;
  }

  return true;
}

void SysInfoNetwork::OnTypeChanged(connection_type_e type, void* user_data) {
  SysInfoNetwork* network = static_cast<SysInfoNetwork*>(user_data);

  if (type == CONNECTION_TYPE_WIFI &&
      network->type_ != SYSTEM_INFO_NETWORK_WIFI)
    network->type_ = SYSTEM_INFO_NETWORK_WIFI;
  else if (!network->GetNetworkType())
    network->type_ = SYSTEM_INFO_NETWORK_NONE;

  picojson::value output = picojson::value(picojson::object());
  picojson::value data = picojson::value(picojson::object());

  network->SetData(data);
  system_info::SetPicoJsonObjectValue(output, "cmd",
      picojson::value("SystemInfoPropertyValueChanged"));
  system_info::SetPicoJsonObjectValue(output, "prop",
      picojson::value("NETWORK"));
  system_info::SetPicoJsonObjectValue(output, "data", data);

  std::string result = output.serialize();
  const char* result_as_cstr = result.c_str();
  AutoLock lock(&(network->events_list_mutex_));
  for (SystemInfoEventsList::iterator it = network_events_.begin();
       it != network_events_.end(); it++) {
    (*it)->PostMessage(result_as_cstr);
  }
}

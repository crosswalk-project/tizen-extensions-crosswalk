// Copyright (c) 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_network_tizen.h"

SysInfoNetworkTizen::SysInfoNetworkTizen() {}

SysInfoNetworkTizen::~SysInfoNetworkTizen() {}

void SysInfoNetworkTizen::StartListening() {
  vconf_notify_key_changed(VCONFKEY_NETWORK_STATUS,
      (vconf_callback_fn)OnNetworkTypeChanged, this);

  vconf_notify_key_changed(VCONFKEY_TELEPHONY_SVCTYPE,
      (vconf_callback_fn)OnCellularNetworkTypeChanged, this);
}

void SysInfoNetworkTizen::StopListening() {
  vconf_ignore_key_changed(VCONFKEY_NETWORK_STATUS,
      (vconf_callback_fn)OnNetworkTypeChanged);

  vconf_ignore_key_changed(VCONFKEY_TELEPHONY_SVCTYPE,
      (vconf_callback_fn)OnCellularNetworkTypeChanged);
}

void SysInfoNetworkTizen::GetCellularNetworkType() {
  int service_type = 0;
  if (vconf_get_int(VCONFKEY_TELEPHONY_SVCTYPE, &service_type)) {
    type_ = SYSTEM_INFO_NETWORK_UNKNOWN;
    return;
  }

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
}

void SysInfoNetworkTizen::OnCellularNetworkTypeChanged(keynode_t* node,
                                                       void* user_data) {
  SysInfoNetworkTizen* network = static_cast<SysInfoNetworkTizen*>(user_data);
  network->GetCellularNetworkType();
  network->SendUpdate();
}

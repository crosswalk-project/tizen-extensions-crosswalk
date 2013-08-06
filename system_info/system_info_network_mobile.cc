// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_network.h"

#include <vconf.h>

void SysInfoNetwork::PlatformInitialize() {
  stopping_ = false;
}

bool SysInfoNetwork::Update(picojson::value& error) {
  int service_type = 0;
  if (vconf_get_int(VCONFKEY_TELEPHONY_SVCTYPE, &service_type)) {
    return false;
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

  return true;
}

gboolean SysInfoNetwork::TimedOutUpdate(gpointer user_data) {
  SysInfoNetwork* instance = static_cast<SysInfoNetwork*>(user_data);
  if (instance->stopping_) {
    instance->stopping_ = false;
    return FALSE;
  }

  SystemInfoNetworkType old_type = instance->type_;
  picojson::value error = picojson::value(picojson::object());;
  if (!instance->Update(error)) {
    // Fail to update, wait for next round
    return TRUE;
  }

  if (old_type != instance->type_) {
    picojson::value output = picojson::value(picojson::object());;
    picojson::value data = picojson::value(picojson::object());

    instance->SetData(data);
    system_info::SetPicoJsonObjectValue(output, "cmd",
        picojson::value("SystemInfoPropertyValueChanged"));
    system_info::SetPicoJsonObjectValue(output, "prop",
        picojson::value("NETWORK"));
    system_info::SetPicoJsonObjectValue(output, "data", data);

    std::string result = output.serialize();
    instance->api_->PostMessage(result.c_str());
  }

  return TRUE;
}

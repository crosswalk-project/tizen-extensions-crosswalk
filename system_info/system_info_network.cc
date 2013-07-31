// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_network.h"

#include "system_info/system_info_utils.h"

using namespace system_info;

void SysInfoNetwork::Get(picojson::value& error,
                         picojson::value& data) {
  if (!Update(error)) {
    if (error.get("message").to_str().empty())
      SetPicoJsonObjectValue(error, "message",
          picojson::value("Get network type faild."));
    return;
  }

  SetData(data);
  SetPicoJsonObjectValue(error, "message", picojson::value(""));
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
    SetPicoJsonObjectValue(output, "cmd",
        picojson::value("SystemInfoPropertyValueChanged"));
    SetPicoJsonObjectValue(output, "prop", picojson::value("NETWORK"));
    SetPicoJsonObjectValue(output, "data", data);
    
    std::string result = output.serialize();
    instance->api_->PostMessage(result.c_str());
  } 
  
  return TRUE;
}

std::string
SysInfoNetwork::ToNetworkTypeString(SystemInfoNetworkType type) {
  std::string ret;
  switch (type) {
    case SYSTEM_INFO_NETWORK_NONE:
      ret = "NONE";
      break;
    case SYSTEM_INFO_NETWORK_2G:
      ret = "2G";
      break;
    case SYSTEM_INFO_NETWORK_2_5G:
      ret = "2.5G";
      break;
    case SYSTEM_INFO_NETWORK_3G:
      ret = "3G";
      break;
    case SYSTEM_INFO_NETWORK_4G:
      ret = "4G";
      break;
    case SYSTEM_INFO_NETWORK_WIFI:
      ret = "WIFI";
      break;
    case SYSTEM_INFO_NETWORK_ETHERNET:
      ret = "ETHERNET";
      break;
    case SYSTEM_INFO_NETWORK_UNKNOWN:
    default:
      ret = "UNKNOWN";
  }

  return ret;
}

void SysInfoNetwork::SetData(picojson::value& data) {
  SetPicoJsonObjectValue(data, "networkType",
      picojson::value(ToNetworkTypeString(type_)));
}

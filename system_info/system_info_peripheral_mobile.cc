// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_peripheral.h"
#include "system_info/system_info_utils.h"

void SysInfoPeripheral::Get(picojson::value& error,
                            picojson::value& data) {
  
 int hdmiStatus = 0, wirelessDisplayStatus = 0 ;
 bool isVideoOutputOn=false;
    if (vconf_get_int(VCONFKEY_MIRACAST_WFD_SOURCE_STATUS, &wirelessDisplayStatus) == 0) {
        if(wirelessDisplayStatus==VCONFKEY_MIRACAST_WFD_SOURCE_ON) {
                isVideoOutputOn = true;
        }
    }
   if (vconf_get_int(VCONFKEY_SYSMAN_HDMI, &hdmiStatus) == 0) {
        if(hdmiStatus==VCONFKEY_SYSMAN_HDMI_CONNECTED) {
                isVideoOutputOn = true;
        }
    }
  system_info::SetPicoJsonObjectValue(data, "isVideoOutputOn",
     picojson::value(isVideoOutputOn));
}

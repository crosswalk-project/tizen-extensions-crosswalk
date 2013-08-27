// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_peripheral.h"

#include "system_info/system_info_utils.h"

#include <vconf.h>

void SysInfoPeripheral::Get(picojson::value& error,
                            picojson::value& data) {
  int hdmiStatus = 0;
  int wirelessDisplayStatus = 0;
  bool isVideoOutputOn = false;

  if (vconf_get_int(VCONFKEY_MIRACAST_WFD_SOURCE_STATUS,
         &wirelessDisplayStatus) == 0) {
        if (wirelessDisplayStatus == VCONFKEY_MIRACAST_WFD_SOURCE_ON) {
                isVideoOutputOn = true;
        }
    } else {
          system_info::SetPicoJsonObjectValue(error, "message",
          picojson::value("Get wireless Display Status faild."));
         return;
    }
  if (vconf_get_int(VCONFKEY_SYSMAN_HDMI, &hdmiStatus) == 0) {
        if (hdmiStatus == VCONFKEY_SYSMAN_HDMI_CONNECTED) {
                isVideoOutputOn = true;
        }
    } else {
          system_info::SetPicoJsonObjectValue(error, "message",
          picojson::value("Get hdmi Status faild."));
         return;
    }
  system_info::SetPicoJsonObjectValue(data, "isVideoOutputOn",
     picojson::value(isVideoOutputOn));
}

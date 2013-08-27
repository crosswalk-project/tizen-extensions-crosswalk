// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_peripheral.h"

#include "system_info/system_info_utils.h"

void SysInfoPeripheral::Get(picojson::value& error,
                            picojson::value& data) {
  isVideoOutputOn = false;
    int wfd = 0;
    int hdmi = 0;

  if (vconf_get_int(VCONFKEY_MIRACAST_WFD_SOURCE_STATUS, &wfd) == 0) {
        if (wfd == VCONFKEY_MIRACAST_WFD_SOURCE_ON) {
                 wfd_ = wfd;
                 isVideoOutputOn = true;
        }
    } else {
          system_info::SetPicoJsonObjectValue(error, "message",
          picojson::value("Get wireless Display Status faild."));
         return;
    }
  if (vconf_get_int(VCONFKEY_SYSMAN_HDMI, &hdmi) == 0) {
        if (hdmi == VCONFKEY_SYSMAN_HDMI_CONNECTED) {
                 hdmi_ = hdmi;
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

void SysInfoPeripheral::updateIsVideoOutputOn() {
    picojson::value output = picojson::value(picojson::object());
    picojson::value data = picojson::value(picojson::object());
    picojson::value error = picojson::value(picojson::object());

    if (wfd_ == VCONFKEY_MIRACAST_WFD_SOURCE_ON ||
                 hdmi_ == VCONFKEY_SYSMAN_HDMI_CONNECTED) {
                isVideoOutputOn = true;
        } else {
                isVideoOutputOn = false;
        }

    Get(error, data);
    system_info::SetPicoJsonObjectValue(output, "cmd",
            picojson::value("SystemInfoPropertyValueChanged"));
    system_info::SetPicoJsonObjectValue(output, "prop",
           picojson::value("PERIPHERAL"));
    system_info::SetPicoJsonObjectValue(output, "data", data);

    std::string result = output.serialize();
    api_->PostMessage(result.c_str());
  }

void SysInfoPeripheral::setWFD(int wfd) {
    wfd_ = wfd;
    updateIsVideoOutputOn();
}

void SysInfoPeripheral::setHDMI(int hdmi) {
    hdmi_ = hdmi;
    updateIsVideoOutputOn();
}

void SysInfoPeripheral::onWFDChanged(keynode_t* node, void* user_data) {
        int wfd= vconf_keynode_get_int(node);
        SysInfoPeripheral* peripheral =
        static_cast<SysInfoPeripheral*>(user_data);

        peripheral->setWFD(wfd);
}

void SysInfoPeripheral::onHDMIChanged(keynode_t* node, void* user_data) {
        int hdmi = vconf_keynode_get_int(node);
        SysInfoPeripheral* peripheral =
        static_cast<SysInfoPeripheral*>(user_data);

        peripheral->setHDMI(hdmi);
}

void SysInfoPeripheral::PlatformInitialize() {
        vconf_notify_key_changed(VCONFKEY_MIRACAST_WFD_SOURCE_STATUS,
                (vconf_callback_fn)onWFDChanged, this);
        vconf_notify_key_changed(VCONFKEY_SYSMAN_HDMI,
                (vconf_callback_fn)onHDMIChanged, this);
}

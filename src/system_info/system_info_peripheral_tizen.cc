// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_peripheral.h"

const std::string SysInfoPeripheral::name_ = "PERIPHERAL";

void SysInfoPeripheral::Get(picojson::value& error,
                            picojson::value& data) {
  if (vconf_get_int(VCONFKEY_MIRACAST_WFD_SOURCE_STATUS, &wfd_) != 0) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get wireless display status failed."));
    return;
  }

  if (vconf_get_int(VCONFKEY_SYSMAN_HDMI, &hdmi_) != 0) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get HDMI status failed."));
    return;
  }

  SendData(data);
  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

void SysInfoPeripheral::SendData(picojson::value& data) {
  is_video_output_ = (wfd_ == VCONFKEY_MIRACAST_WFD_SOURCE_ON) ||
                     (hdmi_ == VCONFKEY_SYSMAN_HDMI_CONNECTED);
  system_info::SetPicoJsonObjectValue(data, "isVideoOutputOn",
      picojson::value(is_video_output_));
}

void SysInfoPeripheral::UpdateIsVideoOutputOn() {
  picojson::value output = picojson::value(picojson::object());
  picojson::value data = picojson::value(picojson::object());

  bool old_is_video_output = is_video_output_;
  SendData(data);
  if (old_is_video_output == is_video_output_)
    return;

  system_info::SetPicoJsonObjectValue(output, "cmd",
      picojson::value("SystemInfoPropertyValueChanged"));
  system_info::SetPicoJsonObjectValue(output, "prop",
      picojson::value("PERIPHERAL"));
  system_info::SetPicoJsonObjectValue(output, "data", data);

  PostMessageToListeners(output);
}

void SysInfoPeripheral::SetWFD(int wfd) {
  wfd_ = wfd;
  UpdateIsVideoOutputOn();
}

void SysInfoPeripheral::SetHDMI(int hdmi) {
  hdmi_ = hdmi;
  UpdateIsVideoOutputOn();
}

void SysInfoPeripheral::OnWFDChanged(keynode_t* node, void* user_data) {
  int wfd = vconf_keynode_get_int(node);
  SysInfoPeripheral* peripheral =
      static_cast<SysInfoPeripheral*>(user_data);

  peripheral->SetWFD(wfd);
}

void SysInfoPeripheral::OnHDMIChanged(keynode_t* node, void* user_data) {
  int hdmi = vconf_keynode_get_int(node);
  SysInfoPeripheral* peripheral =
      static_cast<SysInfoPeripheral*>(user_data);

  peripheral->SetHDMI(hdmi);
}

void SysInfoPeripheral::StartListening() {
  vconf_notify_key_changed(VCONFKEY_MIRACAST_WFD_SOURCE_STATUS,
      (vconf_callback_fn)OnWFDChanged, this);
  vconf_notify_key_changed(VCONFKEY_SYSMAN_HDMI,
      (vconf_callback_fn)OnHDMIChanged, this);
}

void SysInfoPeripheral::StopListening() {
  vconf_ignore_key_changed(VCONFKEY_MIRACAST_WFD_SOURCE_STATUS,
      (vconf_callback_fn)OnWFDChanged);
  vconf_ignore_key_changed(VCONFKEY_SYSMAN_HDMI,
      (vconf_callback_fn)OnHDMIChanged);
}

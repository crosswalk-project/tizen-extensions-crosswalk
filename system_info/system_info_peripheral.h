// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_PERIPHERAL_H_
#define SYSTEM_INFO_SYSTEM_INFO_PERIPHERAL_H_

#if defined(TIZEN_MOBILE)
#include <vconf.h>
#include <vconf-keys.h>
#endif

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"

class SysInfoPeripheral {
 public:
  explicit SysInfoPeripheral(ContextAPI* api)
    :isRegister_(false) {
    api_ = api;
  }
  ~SysInfoPeripheral() {
    if (isRegister_)
      StopListening();
}
  void Get(picojson::value& error, picojson::value& data); //NOLINT
  void StartListening();
  void StopListening();

 private:
#if defined(TIZEN_MOBILE)
  void SetWFD(int wfd);
  void SetHDMI(int hdmi);

  void UpdateIsVideoOutputOn();
  void SendData(picojson::value& data); //NOLINT

  static void OnWFDChanged(keynode_t* node, void* user_data);
  static void OnHDMIChanged(keynode_t* node, void* user_data);
#endif

  ContextAPI* api_;
  bool is_video_output_;
  int wfd_;
  int hdmi_;
  bool isRegister_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoPeripheral);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_PERIPHERAL_H_

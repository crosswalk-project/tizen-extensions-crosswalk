// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_PERIPHERAL_H_
#define SYSTEM_INFO_SYSTEM_INFO_PERIPHERAL_H_

#if defined(TIZEN)
#include <vconf.h>
#include <vconf-keys.h>
#endif

#include <string>

#include "common/picojson.h"
#include "common/utils.h"
#include "system_info/system_info_instance.h"
#include "system_info/system_info_utils.h"

class SysInfoPeripheral : public SysInfoObject {
 public:
  static SysInfoObject& GetInstance() {
    static SysInfoPeripheral instance;
    return instance;
  }
  ~SysInfoPeripheral() {}
  void Get(picojson::value& error, picojson::value& data);
  void StartListening();
  void StopListening();

  static const std::string name_;

 private:
  SysInfoPeripheral() {}

#if defined(TIZEN)
  void SetWFD(int wfd);
  void SetHDMI(int hdmi);

  void UpdateIsVideoOutputOn();
  void SendData(picojson::value& data);

  static void OnWFDChanged(keynode_t* node, void* user_data);
  static void OnHDMIChanged(keynode_t* node, void* user_data);
#endif

  bool is_video_output_;
  int wfd_;
  int hdmi_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoPeripheral);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_PERIPHERAL_H_

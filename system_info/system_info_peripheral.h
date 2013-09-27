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
#include "system_info/system_info_utils.h"

class SysInfoPeripheral {
 public:
  static SysInfoPeripheral& GetSysInfoPeripheral() {
    static SysInfoPeripheral instance;
    return instance;
  }
  ~SysInfoPeripheral() {
    for (SystemInfoEventsList::iterator it = peripheral_events_.begin();
         it != peripheral_events_.end(); it++)
      StopListening(*it);
    pthread_mutex_destroy(&events_list_mutex_);
  }
  void Get(picojson::value& error, picojson::value& data);
  void StartListening(ContextAPI* api);
  void StopListening(ContextAPI* api);

 private:
  explicit SysInfoPeripheral() {
    pthread_mutex_init(&events_list_mutex_, NULL);
  }

#if defined(TIZEN_MOBILE)
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
  pthread_mutex_t events_list_mutex_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoPeripheral);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_PERIPHERAL_H_

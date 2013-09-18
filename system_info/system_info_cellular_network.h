// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_CELLULAR_NETWORK_H_
#define SYSTEM_INFO_SYSTEM_INFO_CELLULAR_NETWORK_H_

#if defined(TIZEN_MOBILE)
#include <vconf-keys.h>
#include <vconf.h>
#endif

#include <string>

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"
#include "system_info/system_info_utils.h"

class SysInfoCellularNetwork {
 public:
  static SysInfoCellularNetwork& GetSysInfoCellularNetwork() {
    static SysInfoCellularNetwork instance;
    return instance;
  }
  ~SysInfoCellularNetwork() {
    for (SystemInfoEventsList::iterator it = cellular_events_.begin();
         it != cellular_events_.end(); it++) {
      StopListening(*it);
    }
    pthread_mutex_destroy(&events_list_mutex_);
  }
  void Get(picojson::value& error, picojson::value& data);
  void StartListening(ContextAPI* api);
  void StopListening(ContextAPI* api);

 private:
  explicit SysInfoCellularNetwork() {
    pthread_mutex_init(&events_list_mutex_, NULL);
  }

#if defined(TIZEN_MOBILE)
  void SendUpdate();
  void SetData(picojson::value& data);

  void SetCellStatus();
  void SetAPN();
  void SetIpAddress();
  void SetMCC();
  void SetMNC();
  void SetCellId();
  void SetLAC();
  void SetIsRoaming();
  void SetFlightMode();
  void SetIMEI();

  void UpdateCellStatus(int status);
  void UpdateIpAddress(char* ip);
  void UpdateCellId(int cell_id);
  void UpdateLAC(int lac);
  void UpdateRoamingState(int is_roaming);
  void UpdateFlightMode(int flight_mode);

  static void OnCellStatusChanged(keynode_t* node, void* user_data);
  static void OnIpChanged(keynode_t* node, void* user_data);
  static void OnCellIdChanged(keynode_t* node, void* user_data);
  static void OnLocationAreaCodeChanged(keynode_t* node, void* user_data);
  static void OnRoamingStateChanged(keynode_t* node, void* user_data);
  static void OnFlightModeChanged(keynode_t* node, void* user_data);
#endif

  std::string status_;
  std::string apn_;
  std::string ipAddress_;
  std::string ipv6Address_;
  int mcc_;
  int mnc_;
  int cellId_;
  int lac_;
  bool isRoaming_;
  bool isFlightMode_;
  std::string imei_;
  pthread_mutex_t events_list_mutex_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoCellularNetwork);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_CELLULAR_NETWORK_H_

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_CELLULAR_NETWORK_H_
#define SYSTEM_INFO_SYSTEM_INFO_CELLULAR_NETWORK_H_

#if defined(TIZEN_MOBILE)
#include <vconf-keys.h>
#include <vconf.h>
#endif

#include <sstream>
#include <string>

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"

class SysInfoCellularNetwork {
 public:
  static SysInfoCellularNetwork& GetSysInfoCellularNetwork(
      ContextAPI* api) {
    static SysInfoCellularNetwork instance(api);
    return instance;
  }
  ~SysInfoCellularNetwork() { }
  void Get(picojson::value& error, picojson::value& data);
  void StartListening();
  void StopListening();

 private:
  explicit SysInfoCellularNetwork(ContextAPI* api) {
    api_ = api;
  }

#if defined(TIZEN_MOBILE)
  void SendUpdate();
  void SetData(picojson::value& data);

  void SetCellStatus();
  void SetIpAddress();
  void SetMcc();
  void SetMnc();
  void SetCellId();
  void SetLac();
  void SetIsRoaming();
  void SetFlightMode();
  void SetImei();

  void UpdateCellStatus(int status);
  void UpdateIpAddress(char* ip);
  void UpdateCellId(int cell_id);
  void UpdateLacChanged(int lac);
  void UpdateRoamingState(int is_roaming);
  void UpdateFlightMode(int flight_mode);

  static void OnCellStatusChanged(keynode_t* node, void* user_data);
  static void OnIpChanged(keynode_t* node, void* user_data);
  static void OnCellIdChanged(keynode_t* node, void* user_data);
  static void OnLacChanged(keynode_t* node, void* user_data);
  static void OnRoamingStateChanged(keynode_t* node, void* user_data);
  static void OnFlightModeChanged(keynode_t* node, void* user_data);
#endif

  ContextAPI* api_;
  std::string status_;
  std::string apn_;
  std::string ipAddress_;
  std::string ipv6Address_;
  std::string mcc_;
  std::string mnc_;
  int cellId_;
  int lac_;
  bool isRoaming_;
  bool isFlightMode_;
  std::string imei_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoCellularNetwork);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_CELLULAR_NETWORK_H_

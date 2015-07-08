// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_CELLULAR_NETWORK_H_
#define SYSTEM_INFO_SYSTEM_INFO_CELLULAR_NETWORK_H_

#if defined(TIZEN_MOBILE)
#include <vconf-keys.h>
#include <vconf.h>
#else
#include <gio/gio.h>
#include <glib.h>
#include <glib-object.h>
#endif

#include <string>

#include "common/picojson.h"
#include "common/utils.h"
#include "system_info/system_info_instance.h"
#include "system_info/system_info_utils.h"

class SysInfoCellularNetwork : public SysInfoObject {
 public:
  static SysInfoObject& GetInstance() {
    static SysInfoCellularNetwork instance;
    return instance;
  }
  ~SysInfoCellularNetwork();
  void Get(picojson::value& error, picojson::value& data);
  void StartListening();
  void StopListening();

  static const std::string name_;

 private:
  SysInfoCellularNetwork();
  void SendUpdate();
  void SetData(picojson::value& data);

#if defined(TIZEN_MOBILE)
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
#else
  GDBusConnection* conn_;
  std::string modem_path_;
  guint connection_manager_watch_;
  guint connection_context_watch_;
  guint network_registration_watch_;

  bool roaming_;
  bool roaming_allowed_;

  void GetCellularNetworkProperties();
  bool GetPropertySuccess(const gchar* object_path,
                          const gchar* interface_name);
  void UpdateCellularNetworkProperty(const gchar* key, GVariant* var_val);
  static void OnCellularNetworkPropertyChanged(
      GDBusConnection* conn, const gchar* sender_name,
      const gchar* object_path, const gchar* iface,
      const gchar* signal_name, GVariant* parameters, gpointer data);
#endif  // TIZEN_MOBILE

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

  DISALLOW_COPY_AND_ASSIGN(SysInfoCellularNetwork);
};
#endif  // SYSTEM_INFO_SYSTEM_INFO_CELLULAR_NETWORK_H_

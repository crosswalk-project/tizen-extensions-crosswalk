// Copyright (c) 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_NETWORK_TIZEN_H_
#define SYSTEM_INFO_SYSTEM_INFO_NETWORK_TIZEN_H_

#include <vconf.h>

#include <string>

#include "system_info/system_info_network.h"

class SysInfoNetworkTizen : public SysInfoNetwork, public SysInfoObject {
 public:
  static SysInfoObject& GetInstance() {
    static SysInfoNetworkTizen instance;
    return instance;
  }
  ~SysInfoNetworkTizen();
  // SysInfoObject
  void Get(picojson::value& error, picojson::value& data) override;
  void StartListening() override;
  void StopListening() override;

 private:
  SysInfoNetworkTizen();
  bool GetNetworkType();
  void GetCellularNetworkType();
  void SendUpdate();

  static void OnNetworkTypeChanged(keynode_t* node, void* user_data);
#ifdef TIZEN_MOBILE
  static void OnCellularNetworkTypeChanged(keynode_t* node, void* user_data);
#else
  void UpdateCellularNetwork(const gchar* key, GVariant* var_val);
  static void OnCellularNetworkTypeChanged(GDBusConnection* conn,
      const gchar* sender_name, const gchar* object_path, const gchar* iface,
      const gchar* signal_name, GVariant* parameters, gpointer data);

  guint cellular_network_type_changed_watch_;
  GDBusConnection* conn_;
  std::string modem_path_;
#endif

  DISALLOW_COPY_AND_ASSIGN(SysInfoNetworkTizen);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_NETWORK_TIZEN_H_

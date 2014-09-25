// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_NETWORK_H_
#define SYSTEM_INFO_SYSTEM_INFO_NETWORK_H_

#if defined(GENERIC_DESKTOP)
#include <gio/gio.h>
#elif defined(TIZEN)
#include <net_connection.h>
#endif
#include <string>

#include "common/picojson.h"
#include "common/utils.h"
#include "system_info/system_info_instance.h"
#include "system_info/system_info_utils.h"

enum SystemInfoNetworkType {
  SYSTEM_INFO_NETWORK_NONE = 0,
  SYSTEM_INFO_NETWORK_2G,
  SYSTEM_INFO_NETWORK_2_5G,
  SYSTEM_INFO_NETWORK_3G,
  SYSTEM_INFO_NETWORK_4G,
  SYSTEM_INFO_NETWORK_WIFI,
  SYSTEM_INFO_NETWORK_ETHERNET,
  SYSTEM_INFO_NETWORK_UNKNOWN
};

#if defined(GENERIC_DESKTOP)
#define G_CALLBACK_1(METHOD, SENDER, ARG0)                                   \
  static void METHOD ## Thunk(SENDER sender, ARG0 res, gpointer userdata) {  \
    return reinterpret_cast<SysInfoNetwork*>(userdata)->METHOD(sender, res); \
  }                                                                          \
                                                                             \
  void METHOD(SENDER, ARG0);
#endif

class SysInfoNetwork : public SysInfoObject {
 public:
  static SysInfoObject& GetInstance() {
    static SysInfoNetwork instance;
    return instance;
  }
  ~SysInfoNetwork();
  void Get(picojson::value& error, picojson::value& data);
  void StartListening();
  void StopListening();

  static const std::string name_;

 private:
  SysInfoNetwork();
  void PlatformInitialize();

  bool Update(picojson::value& error);
  void SetData(picojson::value& data);
  std::string ToNetworkTypeString(SystemInfoNetworkType type);

  SystemInfoNetworkType type_;

#if defined(GENERIC_DESKTOP)
  G_CALLBACK_1(OnNetworkManagerCreated, GObject*, GAsyncResult*);
  G_CALLBACK_1(OnActiveConnectionCreated, GObject*, GAsyncResult*);
  G_CALLBACK_1(OnDevicesCreated, GObject*, GAsyncResult*);

  void UpdateActiveConnection(GVariant* value);
  void UpdateActiveDevice(GVariant* value);
  void UpdateDeviceType(GVariant* value);
  void SendUpdate(guint new_device_type);

  static void OnNetworkManagerSignal(GDBusProxy* proxy, gchar* sender,
      gchar* signal, GVariant* parameters, gpointer user_data);
  static void OnActiveConnectionsSignal(GDBusProxy* proxy, gchar* sender,
      gchar* signal, GVariant* parameters, gpointer user_data);

  SystemInfoNetworkType ToNetworkType(guint device_type);

  std::string active_connection_;
  std::string active_device_;
  guint device_type_;
#elif defined(TIZEN)
  bool GetNetworkType();
  static void OnTypeChanged(connection_type_e type, void* user_data);

  connection_h connection_handle_;
#endif

  DISALLOW_COPY_AND_ASSIGN(SysInfoNetwork);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_NETWORK_H_

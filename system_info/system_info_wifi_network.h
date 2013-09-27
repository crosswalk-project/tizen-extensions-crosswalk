// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_WIFI_NETWORK_H_
#define SYSTEM_INFO_SYSTEM_INFO_WIFI_NETWORK_H_

#if defined(GENERIC_DESKTOP)
#include <gio/gio.h>
#elif defined(TIZEN_MOBILE)
#include <net_connection.h>
#endif
#include <string>

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"
#include "system_info/system_info_utils.h"

#if defined(GENERIC_DESKTOP)
#define G_CALLBACK_WIFI(METHOD, SENDER, ARG0)                                \
  static void METHOD ## Thunk(SENDER sender, ARG0 res, gpointer userdata) {  \
    return reinterpret_cast<SysInfoWifiNetwork*>(userdata)->METHOD(sender,   \
                                                                   res);     \
  }                                                                          \
                                                                             \
  void METHOD(SENDER, ARG0);
#endif

class SysInfoWifiNetwork {
 public:
  static SysInfoWifiNetwork& GetSysInfoWifiNetwork() {
    static SysInfoWifiNetwork instance;
    return instance;
  }
  ~SysInfoWifiNetwork();
  void Get(picojson::value& error, picojson::value& data);
  void StartListening(ContextAPI* api);
  void StopListening(ContextAPI* api);

 private:
  explicit SysInfoWifiNetwork();
  void PlatformInitialize();

  bool Update(picojson::value& error);
  void SendUpdate();
  void SetData(picojson::value& data);

  double signal_strength_;
  std::string ip_address_;
  std::string ipv6_address_;
  std::string ssid_;
  std::string status_;
  pthread_mutex_t events_list_mutex_;

#if defined(GENERIC_DESKTOP)
  G_CALLBACK_WIFI(OnAccessPointCreated, GObject*, GAsyncResult*);
  G_CALLBACK_WIFI(OnActiveAccessPointCreated, GObject*, GAsyncResult*);
  G_CALLBACK_WIFI(OnActiveConnectionCreated, GObject*, GAsyncResult*);
  G_CALLBACK_WIFI(OnDevicesCreated, GObject*, GAsyncResult*);
  G_CALLBACK_WIFI(OnNetworkManagerCreated, GObject*, GAsyncResult*);
  G_CALLBACK_WIFI(OnIPAddressCreated, GObject*, GAsyncResult*);
  G_CALLBACK_WIFI(OnIPv6AddressCreated, GObject*, GAsyncResult*);
  G_CALLBACK_WIFI(OnUpdateIPv6Address, GObject*, GAsyncResult*);

  std::string IPAddressConverter(unsigned int ip);
  void UpdateActiveAccessPoint(GVariant* value);
  void UpdateActiveConnection(GVariant* value);
  void UpdateActiveDevice(GVariant* value);
  void UpdateDeviceType(GVariant* value);
  void UpdateIPAddress(GVariant* value);
  void UpdateIPv6Address(GVariant* value);
  void UpdateSignalStrength(GVariant* value);
  void UpdateSSID(GVariant* value);
  void UpdateStatus(guint new_device_type);

  static void OnAccessPointSignal(GDBusProxy* proxy, gchar* sender,
      gchar* signal, GVariant* parameters, gpointer user_data);
  static void OnActiveConnectionsSignal(GDBusProxy* proxy, gchar* sender,
      gchar* signal, GVariant* parameters, gpointer user_data);
  static void OnNetworkManagerSignal(GDBusProxy* proxy, gchar* sender,
      gchar* signal, GVariant* parameters, gpointer user_data);

  guint device_type_;
  std::string active_access_point_;
  std::string active_connection_;
  std::string active_device_;
  std::string ipv6_config_;
  unsigned int ip_address_desktop_;
#elif defined(TIZEN_MOBILE)
  bool GetIPv4Address();
  bool GetIPv6Address();
  bool GetSignalStrength();
  bool GetSSID();
  bool GetType();
  static void OnIPChanged(const char* ipv4_addr, const char* ipv6_addr,
      void* user_data);
  static void OnTypeChanged(connection_type_e type, void* user_data);

  connection_h connection_handle_;
  connection_profile_h connection_profile_handle_;
#endif

  DISALLOW_COPY_AND_ASSIGN(SysInfoWifiNetwork);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_WIFI_NETWORK_H_

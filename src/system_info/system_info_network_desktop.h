// Copyright (c) 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_NETWORK_DESKTOP_H_
#define SYSTEM_INFO_SYSTEM_INFO_NETWORK_DESKTOP_H_

#include <gio/gio.h>

#include <string>

#include "system_info/system_info_network.h"

#define G_CALLBACK_1(METHOD, SENDER, ARG0)                           \
  static void METHOD ## Thunk(SENDER sender,                         \
                              ARG0 res, gpointer userdata) {         \
    return reinterpret_cast<SysInfoNetworkDesktop*>                  \
        (userdata)->METHOD(sender, res);                             \
  }                                                                  \
                                                                     \
  void METHOD(SENDER, ARG0);

class SysInfoNetworkDesktop : public SysInfoNetwork, public SysInfoObject {
 public:
  static SysInfoObject& GetInstance() {
    static SysInfoNetworkDesktop instance;
    return instance;
  }
  ~SysInfoNetworkDesktop();
  // SysInfoObject
  void Get(picojson::value& error, picojson::value& data) override;
  void StartListening() override;
  void StopListening() override;

 private:
  SysInfoNetworkDesktop();
  SystemInfoNetworkType ToNetworkType(guint device_type);

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

  std::string active_connection_;
  std::string active_device_;
  guint device_type_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoNetworkDesktop);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_NETWORK_DESKTOP_H_

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_NETWORK_H_
#define SYSTEM_INFO_SYSTEM_INFO_NETWORK_H_

#if defined(GENERIC_DESKTOP)
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#endif
#include <glib.h>
#include <string>

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"
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

class SysInfoNetwork {
 public:
  static SysInfoNetwork& GetSysInfoNetwork(
      ContextAPI* api) {
    static SysInfoNetwork instance(api);
    return instance;
  }
  ~SysInfoNetwork();
  void Get(picojson::value& error, picojson::value& data);
  inline void StartListen() {
    stopping_ = false;

    // FIXME(halton): Use udev D-Bus interface to monitor.
    g_timeout_add(system_info::default_timeout_interval,
                  SysInfoNetwork::TimedOutUpdate,
                  static_cast<gpointer>(this));
  }
  inline void StopListen() {  stopping_ = true; }

 private:
  explicit SysInfoNetwork(ContextAPI* api);

  static gboolean TimedOutUpdate(gpointer user_data);
  bool Update(picojson::value& error);
  void SetData(picojson::value& data);
  std::string ToNetworkTypeString(SystemInfoNetworkType type);

  ContextAPI* api_;

  SystemInfoNetworkType type_;
  bool stopping_;

#if defined(GENERIC_DESKTOP)
  static void OnNMStateChanged(DBusGProxy* proxy,
                               guint new_state,
                               guint old_state,
                               guint reason,
                               gpointer user_data);
  SystemInfoNetworkType ToNetworkType(guint device_type);
  DBusGProxy* dbus_prop_proxy_;
  DBusGProxy* dbus_nm_proxy_;
  guint       nm_device_type_;
#endif

  DISALLOW_COPY_AND_ASSIGN(SysInfoNetwork);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_NETWORK_H_

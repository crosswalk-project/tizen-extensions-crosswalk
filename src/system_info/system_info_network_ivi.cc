// Copyright (c) 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_network_tizen.h"

SysInfoNetworkTizen::SysInfoNetworkTizen() {
  conn_ = system_info::GetDbusConnection();
  if (!conn_)
    return;
  modem_path_ = system_info::OfonoGetModemPath(conn_);
}

SysInfoNetworkTizen::~SysInfoNetworkTizen() {
  if (!conn_)
    return;
  g_dbus_connection_signal_unsubscribe(conn_,
      cellular_network_type_changed_watch_);
  g_dbus_connection_close_sync(conn_, NULL, NULL);
  conn_ = NULL;
}

void SysInfoNetworkTizen::StartListening() {
  vconf_notify_key_changed(VCONFKEY_NETWORK_STATUS,
      (vconf_callback_fn)OnNetworkTypeChanged, this);

  if (!conn_ || modem_path_.empty())
    return;
  cellular_network_type_changed_watch_ = g_dbus_connection_signal_subscribe(
      conn_, system_info::kOfonoService,
      system_info::kOfonoNetworkRegistrationIface, "PropertyChanged",
      modem_path_.c_str(), NULL, G_DBUS_SIGNAL_FLAGS_NONE,
      OnCellularNetworkTypeChanged, this, NULL);
}

void SysInfoNetworkTizen::StopListening() {
  vconf_ignore_key_changed(VCONFKEY_NETWORK_STATUS,
      (vconf_callback_fn)OnNetworkTypeChanged);

  if (!conn_)
    return;
  g_dbus_connection_signal_unsubscribe(conn_,
      cellular_network_type_changed_watch_);
}

void SysInfoNetworkTizen::GetCellularNetworkType() {
  if (!conn_ || modem_path_.empty()) {
    type_ = SYSTEM_INFO_NETWORK_UNKNOWN;
    return;
  }

  GError* error = NULL;
  GVariant* var = g_dbus_connection_call_sync(conn_, system_info::kOfonoService,
      modem_path_.c_str(), system_info::kOfonoNetworkRegistrationIface,
      "GetProperties", NULL, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

  if (!var) {
    std::cout << "Ofono network GetProperties failed: "
              << error->message << std::endl;
    g_error_free(error);
    type_ = SYSTEM_INFO_NETWORK_UNKNOWN;
    return;
  }

  GVariantIter* iter;
  g_variant_get(var, "(a{sv})", &iter);
  gchar* key;
  GVariant* var_val;
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    UpdateCellularNetwork(key, var_val);
    g_free(key);
    g_variant_unref(var_val);
  }

  g_variant_unref(var);
  g_variant_iter_free(iter);
}

void SysInfoNetworkTizen::UpdateCellularNetwork(const gchar* key,
                                                GVariant* var_val) {
  if (g_strcmp0(key, "Technology") != 0)
    return;

  const char* val = g_variant_get_string(var_val, NULL);
  if (!val) {
    type_ = SYSTEM_INFO_NETWORK_NONE;
    return;
  } else if (g_strcmp0(val, "gsm") == 0) {
    type_ = SYSTEM_INFO_NETWORK_2G;
  } else if (g_strcmp0(val, "edge") == 0) {
    type_ = SYSTEM_INFO_NETWORK_2_5G;
  } else if (g_strcmp0(val, "umts") == 0) {
    type_ = SYSTEM_INFO_NETWORK_3G;
  } else if (g_strcmp0(val, "hspa") == 0 || g_strcmp0(val, "lte") == 0) {
    type_ = SYSTEM_INFO_NETWORK_4G;
  } else {
    type_ = SYSTEM_INFO_NETWORK_UNKNOWN;
  }
}

void SysInfoNetworkTizen::OnCellularNetworkTypeChanged(GDBusConnection* conn,
    const gchar* sender_name, const gchar* object_path, const gchar* iface,
    const gchar* signal_name, GVariant* parameters, gpointer data) {
  gchar* key;
  GVariant* value;
  g_variant_get(parameters, "(sv)", &key, &value);
  SysInfoNetworkTizen* network = static_cast<SysInfoNetworkTizen*>(data);
  network->UpdateCellularNetwork(key, value);
  g_free(key);
  g_variant_unref(value);
  network->SendUpdate();
}

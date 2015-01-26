// Copyright (c) 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_cellular_network.h"

SysInfoCellularNetwork::SysInfoCellularNetwork()
    : status_(""),
      apn_(""),
      ipAddress_(""),
      ipv6Address_(""),
      mcc_(0),
      mnc_(0),
      cellId_(0),
      lac_(0),
      isRoaming_(false),
      // FIXME(liying): NOT SUPPORTED
      isFlightMode_(false),
      imei_(""),
      roaming_(false),
      roaming_allowed_(false) {
  conn_ = system_info::GetDbusConnection();
  if (!conn_)
    return;
  modem_path_ = system_info::OfonoGetModemPath(conn_);
}

SysInfoCellularNetwork::~SysInfoCellularNetwork() {
  if (!conn_)
    return;
  g_dbus_connection_signal_unsubscribe(conn_, connection_manager_watch_);
  g_dbus_connection_signal_unsubscribe(conn_, connection_context_watch_);
  g_dbus_connection_signal_unsubscribe(conn_, network_registration_watch_);
  g_dbus_connection_close_sync(conn_, NULL, NULL);
  conn_ = NULL;
}

void SysInfoCellularNetwork::Get(picojson::value& error,
                                 picojson::value& data) {
  GetCellularNetworkProperties();
  SetData(data);
  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

void SysInfoCellularNetwork::GetCellularNetworkProperties() {
  if (!conn_ || modem_path_.empty())
    return;
  // roaming_, lac_, cellId_, mcc_, mnc_
  if (!GetPropertySuccess(modem_path_.c_str(),
                          system_info::kOfonoNetworkRegistrationIface))
    return;
  // roaming_allowed_
  if (!GetPropertySuccess(modem_path_.c_str(),
                          system_info::kOfonoConnectionManagerIface))
    return;
  // status_, imei_
  if (!GetPropertySuccess(modem_path_.c_str(), system_info::kOfonoModemIface))
    return;

  GError* error = NULL;
  GVariant* var_contexts_properties = g_dbus_connection_call_sync(
      conn_, system_info::kOfonoService, modem_path_.c_str(),
      system_info::kOfonoConnectionManagerIface, "GetContexts", NULL, NULL,
      G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
  if (!var_contexts_properties) {
    std::cout << "dbus call failed: " << error->message << std::endl;
    g_error_free(error);
    return;
  }
  GVariantIter* iter;
  g_variant_get(var_contexts_properties, "(a(oa{sv}))", &iter);
  g_variant_unref(var_contexts_properties);
  gchar* path;
  while (g_variant_iter_next(iter, "(o@a{sv})", &path, NULL)) {
    if (!path)
      continue;
    // apn_, ipAddress_, ipv6Address_
    bool flag = GetPropertySuccess(path,
                                   system_info::kOfonoConnectionContextIface);
    g_free(path);
    if (flag)
      break;
  }
  g_variant_iter_free(iter);
}

bool SysInfoCellularNetwork::GetPropertySuccess(const char* object_path,
                                                const char* interface_name) {
  GError* error = NULL;
  GVariant* var = g_dbus_connection_call_sync(conn_, system_info::kOfonoService,
      object_path, interface_name, "GetProperties", NULL, NULL,
      G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
  if (!var) {
    std::cout << "dbus call failed: " << error->message << std::endl;
    g_error_free(error);
    return false;
  }

  GVariantIter *iter;
  g_variant_get(var, "(a{sv})", &iter);
  g_variant_unref(var);
  gchar* key;
  GVariant* var_val;
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    UpdateCellularNetworkProperty(key, var_val);
    g_free(key);
    g_variant_unref(var_val);
  }
  g_variant_iter_free(iter);

  return true;
}

void SysInfoCellularNetwork::UpdateCellularNetworkProperty(
    const gchar* key, GVariant* var_val) {
  const char* val;
  if (g_strcmp0(key, "MobileCountryCode") == 0) {
    val = g_variant_get_string(var_val, NULL);
    mcc_ = val ? strtoul(val, NULL, 0) : 0;
  } else if (g_strcmp0(key, "MobileNetworkCode") == 0) {
    val = g_variant_get_string(var_val, NULL);
    mnc_ = val ? strtoul(val, NULL, 0) : 0;
  } else if (g_strcmp0(key, "CellId") == 0) {
    val = g_variant_get_string(var_val, NULL);
    cellId_ = val ? strtoul(val, NULL, 0) : 0;
  } else if (g_strcmp0(key, "LocationAreaCode") == 0) {
    val = g_variant_get_string(var_val, NULL);
    lac_ = val ? strtoul(val, NULL, 0) : 0;
  } else if (g_strcmp0(key, "Status") == 0) {
    val = g_variant_get_string(var_val, NULL);
    roaming_ = g_strcmp0(val, "roaming") ? false :true;
  } else if (g_strcmp0(key, "RoamingAllowed") == 0) {
    roaming_allowed_ = g_variant_get_boolean(var_val);
  } else if (g_strcmp0(key, "Online") == 0) {
    status_ = g_variant_get_boolean(var_val) ? "ON" : "OFF";
  } else if (g_strcmp0(key, "Serial") == 0) {
    imei_ = g_variant_get_string(var_val, NULL);
  } else if (g_strcmp0(key, "AccessPointName") == 0) {
    apn_ = g_variant_get_string(var_val, NULL);
  } else if (g_strcmp0(key, "Settings") == 0) {
    GVariantIter* v4_iter;
    g_variant_get(var_val, "a{sv}", &v4_iter);
    gchar* k;
    GVariant* v;
    while (g_variant_iter_next(v4_iter, "{sv}", &k, &v)) {
      if (g_strcmp0(k, "Address") == 0)
        ipAddress_ = g_variant_get_string(v, NULL);
      g_free(k);
      g_variant_unref(v);
      break;
    }
    g_variant_iter_free(v4_iter);
  } else if (g_strcmp0(key, "IPv6.Settings") == 0) {
    GVariantIter* v6_iter;
    g_variant_get(var_val, "a{sv}", &v6_iter);
    gchar* k;
    GVariant* v;
    while (g_variant_iter_next(v6_iter, "{sv}", &k, &v)) {
      if (g_strcmp0(k, "Address") == 0)
        ipv6Address_ = g_variant_get_string(v, NULL);
      g_free(k);
      g_variant_unref(v);
      break;
    }
    g_variant_iter_free(v6_iter);
  }
  isRoaming_ = (roaming_ && roaming_allowed_);
}

void SysInfoCellularNetwork::OnCellularNetworkPropertyChanged(
    GDBusConnection* conn, const gchar* sender_name,
    const gchar* object_path, const gchar* iface,
    const gchar* signal_name, GVariant* parameters, gpointer data) {
  gchar* key;
  GVariant* value;
  g_variant_get(parameters, "(sv)", &key, &value);
  SysInfoCellularNetwork* cellularNetwork =
      static_cast<SysInfoCellularNetwork*>(data);
  cellularNetwork->UpdateCellularNetworkProperty(key, value);
  g_free(key);
  g_variant_unref(value);
  cellularNetwork->SendUpdate();
}

void SysInfoCellularNetwork::StartListening() {
  if (!conn_)
    return;

  connection_manager_watch_ = g_dbus_connection_signal_subscribe(conn_,
      system_info::kOfonoService, system_info::kOfonoConnectionManagerIface,
      "PropertyChanged", modem_path_.c_str(), NULL, G_DBUS_SIGNAL_FLAGS_NONE,
      OnCellularNetworkPropertyChanged, this, NULL);

  connection_context_watch_ = g_dbus_connection_signal_subscribe(conn_,
     system_info::kOfonoService, system_info::kOfonoConnectionContextIface,
     "PropertyChanged", modem_path_.c_str(), NULL, G_DBUS_SIGNAL_FLAGS_NONE,
     OnCellularNetworkPropertyChanged, this, NULL);

  network_registration_watch_ = g_dbus_connection_signal_subscribe(conn_,
     system_info::kOfonoService, system_info::kOfonoNetworkRegistrationIface,
     "PropertyChanged", modem_path_.c_str(), NULL, G_DBUS_SIGNAL_FLAGS_NONE,
     OnCellularNetworkPropertyChanged, this, NULL);
}

void SysInfoCellularNetwork::StopListening() {
  if (!conn_)
    return;
  g_dbus_connection_signal_unsubscribe(conn_, connection_manager_watch_);
  g_dbus_connection_signal_unsubscribe(conn_, connection_context_watch_);
  g_dbus_connection_signal_unsubscribe(conn_, network_registration_watch_);
}

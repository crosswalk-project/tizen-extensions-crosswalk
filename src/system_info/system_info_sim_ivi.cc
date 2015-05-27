// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_sim.h"

SysInfoSim::SysInfoSim()
    : state_(SYSTEM_INFO_SIM_UNKNOWN),
      operator_name_(""),
      msisdn_(""),
      iccid_(""),
      mcc_(0),
      mnc_(0),
      msin_(""),
      spn_(""),
      conn_(NULL) {
  conn_ = system_info::GetDbusConnection();
  if (!conn_)
    return;
  modem_path_ = system_info::OfonoGetModemPath(conn_);
}

SysInfoSim::~SysInfoSim() {
  if (!conn_)
    return;
  g_dbus_connection_signal_unsubscribe(conn_, prop_changed_watch_);
  g_dbus_connection_close_sync(conn_, NULL, NULL);
  conn_ = NULL;
}

void SysInfoSim::Get(picojson::value& error,
                     picojson::value& data) {
  GetSimProperties();
  GetOperatorNameAndSpn();
  SetJsonValues(data);
  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

void SysInfoSim::UpdateSimProperty(const gchar* key, GVariant* var_val) {
  const char* val;
  if (g_strcmp0(key, "Present") == 0) {
    state_ = g_variant_get_boolean(var_val) ?
             SYSTEM_INFO_SIM_UNKNOWN : SYSTEM_INFO_SIM_ABSENT;
  } else if (g_strcmp0(key, "SubscriberIdentity") == 0) {
    msin_ = g_variant_get_string(var_val, NULL);
  } else if (g_strcmp0(key, "CardIdentifier") == 0) {
    iccid_ = g_variant_get_string(var_val, NULL);
  } else if (g_strcmp0(key, "MobileCountryCode") == 0) {
    val = g_variant_get_string(var_val, NULL);
    mcc_ = val ? strtoul(val, NULL, 0) : 0;
  } else if (g_strcmp0(key, "MobileNetworkCode") == 0) {
    val = g_variant_get_string(var_val, NULL);
    mnc_ = val ? strtoul(val, NULL, 0) : 0;
  } else if (g_strcmp0(key, "SubscriberNumbers") == 0) {
    GVariantIter* msisdn_iter;
    char* num = NULL;
    g_variant_get(var_val, "as", &msisdn_iter);
    g_variant_iter_next(msisdn_iter, "s", &num);
    msisdn_ = num ? num : "";
    g_free(num);
    g_variant_iter_free(msisdn_iter);
  } else if (g_strcmp0(key, "PinRequired") == 0) {
    const char* lock = g_variant_get_string(var_val, NULL);
    if (!lock)
      state_ = SYSTEM_INFO_SIM_UNKNOWN;
    else if (g_strcmp0(lock, "none") == 0)
      state_ = SYSTEM_INFO_SIM_READY;
    else if (g_strcmp0(lock, "pin") == 0 ||
             g_strcmp0(lock, "pin2") == 0)
      state_ = SYSTEM_INFO_SIM_PIN_REQUIRED;
    else if (g_strcmp0(lock, "puk") == 0 ||
             g_strcmp0(lock, "puk2") == 0)
      state_ = SYSTEM_INFO_SIM_PUB_REQUIRED;
    else if (g_strcmp0(lock, "network") == 0)
      state_ = SYSTEM_INFO_SIM_NETWORK_LOCKED;
    else if (g_strcmp0(lock, "networkpuk") == 0 ||
             g_strcmp0(lock, "phone") == 0 ||
             g_strcmp0(lock, "firstphone") == 0 ||
             g_strcmp0(lock, "firstphonepuk") == 0 ||
             g_strcmp0(lock, "netsub") == 0 ||
             g_strcmp0(lock, "netsubpuk") == 0 ||
             g_strcmp0(lock, "service") == 0 ||
             g_strcmp0(lock, "servicepuk") == 0 ||
             g_strcmp0(lock, "corp") == 0 ||
             g_strcmp0(lock, "corppuk") == 0)
      state_ = SYSTEM_INFO_SIM_UNKNOWN;
    else
      state_ = SYSTEM_INFO_SIM_INITIALIZING;
  }
}

void SysInfoSim::OnSimPropertyChanged(GDBusConnection* conn,
                                      const gchar* sender_name,
                                      const gchar* object_path,
                                      const gchar* iface,
                                      const gchar* signal_name,
                                      GVariant* parameters,
                                      gpointer data) {
  gchar* key;
  GVariant* value;
  g_variant_get(parameters, "(sv)", &key, &value);
  SysInfoSim* sim = static_cast<SysInfoSim*>(data);
  sim->UpdateSimProperty(key, value);
  g_free(key);
  g_variant_unref(value);
  sim->GetOperatorNameAndSpn();
}

void SysInfoSim::GetSimProperties() {
  if (!conn_ || modem_path_.empty())
    return;
  GError* error = NULL;
  GVariant* var_properties = g_dbus_connection_call_sync(
      conn_, system_info::kOfonoService, modem_path_.c_str(),
      system_info::kOfonoSimManagerIface, "GetProperties", NULL, NULL,
      G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

  if (!var_properties) {
    std::cout << "dbus call failed: " << error->message << std::endl;
    g_error_free(error);
    return;
  }

  GVariantIter* iter;
  gchar* key;
  GVariant* var_val;
  g_variant_get(var_properties, "(a{sv})", &iter);
  g_variant_unref(var_properties);
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    UpdateSimProperty(key, var_val);
    g_free(key);
    g_variant_unref(var_val);
  }

  g_variant_iter_free(iter);
}

void SysInfoSim::GetOperatorNameAndSpn() {
  if (!conn_ || modem_path_.empty())
    return;
  GError* error = NULL;
  GVariant* var_properties = g_dbus_connection_call_sync(
      conn_, system_info::kOfonoService, modem_path_.c_str(),
      system_info::kOfonoNetworkRegistrationIface, "GetProperties", NULL, NULL,
      G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

  if (!var_properties) {
    std::cout << "dbus call failed: " << error->message << std::endl;
    g_error_free(error);
    return;
  }

  GVariantIter* iter;
  gchar* key;
  GVariant* var_val;
  int net_mcc = 0;
  int net_mnc = 0;
  g_variant_get(var_properties, "(a{sv})", &iter);
  g_variant_unref(var_properties);
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    if (g_strcmp0(key, "Name") == 0) {
      char* name = NULL;
      g_variant_get(var_val, "s", &name);
      operator_name_ = name ? name : "";
      g_free(name);
    } else if (g_strcmp0(key, "MobileCountryCode") == 0) {
      const char* mcc = g_variant_get_string(var_val, NULL);
      net_mcc = mcc ? strtoul(mcc, NULL, 0) : 0;
    } else if (g_strcmp0(key, "MobileNetworkCode") == 0) {
      const char* mnc = g_variant_get_string(var_val, NULL);
      net_mnc = mnc ? strtoul(mnc, NULL, 0) : 0;
    }
    g_free(key);
    g_variant_unref(var_val);
  }
  g_variant_iter_free(iter);
  spn_ = net_mcc + net_mnc ? std::to_string(net_mcc + net_mnc) : "";
}

void SysInfoSim::StartListening() {
  if (!conn_ || modem_path_.empty())
    return;
  prop_changed_watch_ = g_dbus_connection_signal_subscribe(
      conn_, system_info::kOfonoService, system_info::kOfonoSimManagerIface,
      "PropertyChanged", modem_path_.c_str(), NULL, G_DBUS_SIGNAL_FLAGS_NONE,
      OnSimPropertyChanged, this, NULL);
}

void SysInfoSim::StopListening() {
  if (!conn_)
    return;
  g_dbus_connection_signal_unsubscribe(conn_, prop_changed_watch_);
}

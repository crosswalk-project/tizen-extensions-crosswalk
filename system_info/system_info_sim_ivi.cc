// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_sim.h"

namespace {

const char kModemPath[] = "/he910_0";
const char kOfonoService[] = "org.ofono";
const char kOfonoSimManagerIface[] = "org.ofono.SimManager";
const char kOfonoNetworkRegistrationIface[] = "org.ofono.NetworkRegistration";

}  // namespace

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
  InitDbusConnection();
}

SysInfoSim::~SysInfoSim() {
  DeInitDbusConnection();
}

void SysInfoSim::Get(picojson::value& error,
                     picojson::value& data) {
  GetSimProperties();
  GetOperatorNameAndSpn();
  SetJsonValues(data);
  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

void SysInfoSim::InitDbusConnection() {
  GError* error = NULL;
  gchar* addr = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                NULL, &error);
  if (!addr) {
    std::cout << "fail to get dbus addr: " << error->message << std::endl;
    g_free(error);
    return;
  }

  conn_ = g_dbus_connection_new_for_address_sync(addr,
      (GDBusConnectionFlags)(G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
      G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION), NULL, NULL, &error);
  if (!conn_) {
    std::cout << "fail to create dbus connection: "
              << error->message << std::endl;
    g_free(error);
    return;
  }
}

void SysInfoSim::DeInitDbusConnection() {
  if (!conn_)
    return;
  g_dbus_connection_signal_unsubscribe(conn_, prop_changed_watch_);
  g_dbus_connection_close_sync(conn_, NULL, NULL);
  conn_ = NULL;
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
    else if (g_strcmp0(lock, "pin") == 0)
      state_ = SYSTEM_INFO_SIM_PIN_REQUIRED;
    else if (g_strcmp0(lock, "puk") == 0)
      state_ = SYSTEM_INFO_SIM_PUB_REQUIRED;
    else if (g_strcmp0(lock, "phone") == 0)
      state_ = SYSTEM_INFO_SIM_UNKNOWN;
    else if (g_strcmp0(lock, "firstphone") == 0)
      state_ = SYSTEM_INFO_SIM_UNKNOWN;
    else if (g_strcmp0(lock, "firstphonepuk") == 0)
      state_ = SYSTEM_INFO_SIM_UNKNOWN;
    else if (g_strcmp0(lock, "pin2") == 0)
      state_ = SYSTEM_INFO_SIM_PIN_REQUIRED;
    else if (g_strcmp0(lock, "puk2") == 0)
      state_ = SYSTEM_INFO_SIM_PUB_REQUIRED;
    else if (g_strcmp0(lock, "network") == 0)
      state_ = SYSTEM_INFO_SIM_NETWORK_LOCKED;
    else if (g_strcmp0(lock, "networkpuk") == 0)
      state_ = SYSTEM_INFO_SIM_UNKNOWN;
    else if (g_strcmp0(lock, "netsub") == 0)
      state_ = SYSTEM_INFO_SIM_UNKNOWN;
    else if (g_strcmp0(lock, "netsubpuk") == 0)
      state_ = SYSTEM_INFO_SIM_UNKNOWN;
    else if (g_strcmp0(lock, "service") == 0)
      state_ = SYSTEM_INFO_SIM_UNKNOWN;
    else if (g_strcmp0(lock, "servicepuk") == 0)
      state_ = SYSTEM_INFO_SIM_UNKNOWN;
    else if (g_strcmp0(lock, "corp") == 0)
      state_ = SYSTEM_INFO_SIM_UNKNOWN;
    else if (g_strcmp0(lock, "corppuk") == 0)
      state_ = SYSTEM_INFO_SIM_UNKNOWN;
    else {
      state_ = SYSTEM_INFO_SIM_INITIALIZING;
    }
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
  if (!conn_)
    return;
  GError* error = NULL;
  GVariant* var_properties = g_dbus_connection_call_sync(
      conn_, kOfonoService, kModemPath,
      kOfonoSimManagerIface, "GetProperties", NULL, NULL,
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
  while (g_variant_iter_next(iter, "{sv}", &key, &var_val)) {
    UpdateSimProperty(key, var_val);
    g_free(key);
    g_variant_unref(var_val);
  }

  g_variant_iter_free(iter);
  g_variant_unref(var_properties);
}

void SysInfoSim::GetOperatorNameAndSpn() {
  if (!conn_)
    return;
  GError* error = NULL;
  GVariant* var_properties = g_dbus_connection_call_sync(
      conn_, kOfonoService, kModemPath,
      kOfonoNetworkRegistrationIface, "GetProperties", NULL, NULL,
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
  g_variant_unref(var_properties);
  spn_ = net_mcc + net_mnc ? std::to_string(net_mcc + net_mnc) : "";
}

void SysInfoSim::StartListening() {
  if (!conn_)
    return;
  prop_changed_watch_ = g_dbus_connection_signal_subscribe(
      conn_, kOfonoService, kOfonoSimManagerIface, "PropertyChanged",
      kModemPath, NULL, G_DBUS_SIGNAL_FLAGS_NONE, OnSimPropertyChanged,
      this, NULL);
}

void SysInfoSim::StopListening() {
  if (!conn_)
    return;
  g_dbus_connection_signal_unsubscribe(conn_, prop_changed_watch_);
}

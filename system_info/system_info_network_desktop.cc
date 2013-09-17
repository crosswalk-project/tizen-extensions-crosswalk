// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_network.h"

#include <NetworkManager.h>

#include "system_info/system_info_utils.h"

namespace {

const char sDBusServiceNM[] = "org.freedesktop.NetworkManager";
const char sManagerPath[] = "/org/freedesktop/NetworkManager";
const char sManagerInterface[] = "org.freedesktop.NetworkManager";
const char sConnectionActiveInterface[] =
    "org.freedesktop.NetworkManager.Connection.Active";
const char sDeviceInterface[] = "org.freedesktop.NetworkManager.Device";

}  // namespace

SysInfoNetwork::SysInfoNetwork(ContextAPI* api)
    : type_(SYSTEM_INFO_NETWORK_UNKNOWN) {
  api_ = api;
  PlatformInitialize();
}

void SysInfoNetwork::PlatformInitialize() {
  active_connection_ = "";
  active_device_ = "";
  device_type_ = NM_DEVICE_TYPE_UNKNOWN;

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
      G_DBUS_PROXY_FLAGS_NONE,
      NULL,
      sDBusServiceNM,
      sManagerPath,
      sManagerInterface,
      NULL,
      OnNetworkManagerCreatedThunk,
      this);
}

SysInfoNetwork::~SysInfoNetwork() {
}

void SysInfoNetwork::StartListening() {
}

void SysInfoNetwork::StopListening() {
}

void SysInfoNetwork::OnNetworkManagerCreated(GObject*, GAsyncResult* res) {
  GError* err = 0;
  GDBusProxy* proxy = g_dbus_proxy_new_for_bus_finish(res, &err);

  if (!proxy) {
    g_printerr("NetworkManager proxy creation error: %s\n", err->message);
    g_error_free(err);
    return;
  }

  GVariant* value = g_dbus_proxy_get_cached_property(proxy,
                                                     "ActiveConnections");
  if (!value) {
    g_printerr("Get ActiveConnections failed.");
    return;
  }
  UpdateActiveConnection(value);

  // FIXME(halton): NetworkManager does not support g-properties-changed signal.
  g_signal_connect(proxy, "g-signal",
      G_CALLBACK(SysInfoNetwork::OnNetworkManagerSignal), this);
}

void SysInfoNetwork::OnActiveConnectionCreated(GObject*, GAsyncResult* res) {
  GError* err = 0;
  GDBusProxy* proxy = g_dbus_proxy_new_for_bus_finish(res, &err);

  if (!proxy) {
    g_printerr("ActiveConnection proxy creation error: %s\n", err->message);
    g_error_free(err);
    return;
  }

  GVariant* value = g_dbus_proxy_get_cached_property(proxy, "Devices");
  if (!value) {
    g_printerr("Failed to get devices.");
    return;
  }
  UpdateActiveDevice(value);

  g_signal_connect(proxy, "g-signal",
      G_CALLBACK(SysInfoNetwork::OnActiveConnectionsSignal), this);
}

void SysInfoNetwork::OnDevicesCreated(GObject*, GAsyncResult* res) {
  GError* err = 0;
  GDBusProxy* proxy = g_dbus_proxy_new_for_bus_finish(res, &err);

  if (!proxy) {
    g_printerr("Devices proxy creation error: %s\n", err->message);
    g_error_free(err);
    return;
  }

  GVariant* value = g_dbus_proxy_get_cached_property(proxy, "DeviceType");
  if (!value) {
    g_printerr("Get DeviceType failed.");
    return;
  }
  UpdateDeviceType(value);
}

bool SysInfoNetwork::Update(picojson::value& error) {
  // type_ will be updated by NM signals
  return true;
}

SystemInfoNetworkType SysInfoNetwork::ToNetworkType(guint device_type) {
  SystemInfoNetworkType ret = SYSTEM_INFO_NETWORK_UNKNOWN;

  switch (device_type) {
    case NM_DEVICE_TYPE_ETHERNET:
      ret = SYSTEM_INFO_NETWORK_ETHERNET;
      break;
    case NM_DEVICE_TYPE_WIFI:
      ret = SYSTEM_INFO_NETWORK_WIFI;
      break;
    case NM_DEVICE_TYPE_MODEM:
      // FIXME(halton): Identify 2G/2.5G/3G/4G
      break;
    case NM_DEVICE_TYPE_UNKNOWN:
    default:
      ret = SYSTEM_INFO_NETWORK_UNKNOWN;
  }

  return ret;
}

void SysInfoNetwork::UpdateActiveConnection(GVariant* value) {
  if (!value || !g_variant_n_children(value)) {
    active_connection_ = "";
    active_device_ = "";
    SendUpdate(NM_DEVICE_TYPE_UNKNOWN);
    return;
  }

  GVariant* child = g_variant_get_child_value(value, 0);
  if (!child) {
    g_printerr("Invalid child[0] for ActiveConnections.");
    SendUpdate(NM_DEVICE_TYPE_UNKNOWN);
    return;
  }

  const char* str = g_variant_get_string(child, NULL);
  if (str && (strcmp(active_connection_.c_str(), str) != 0)) {
    active_connection_ = std::string(str);
    active_device_ = "";
    SendUpdate(NM_DEVICE_TYPE_UNKNOWN);
  }

  if (active_connection_.empty())
    return;

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
      G_DBUS_PROXY_FLAGS_NONE,
      NULL,
      sDBusServiceNM,
      active_connection_.c_str(),
      sConnectionActiveInterface,
      NULL,
      OnActiveConnectionCreatedThunk,
      this);
}

void SysInfoNetwork::UpdateActiveDevice(GVariant* value) {
  if (!value || !g_variant_n_children(value)) {
    active_device_ = "";
    SendUpdate(NM_DEVICE_TYPE_UNKNOWN);
    return;
  }

  GVariant* child = g_variant_get_child_value(value, 0);
  if (!child) {
    g_printerr("Invalid child[0] for Devices.");
    SendUpdate(NM_DEVICE_TYPE_UNKNOWN);
    return;
  }

  const char* str = g_variant_get_string(child, NULL);
  if (str && (strcmp(active_device_.c_str(), str) != 0)) {
    active_device_ = std::string(str);
    SendUpdate(NM_DEVICE_TYPE_UNKNOWN);
  }

  if (active_device_.empty())
    return;

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
      G_DBUS_PROXY_FLAGS_NONE,
      NULL,
      sDBusServiceNM,
      active_device_.c_str(),
      sDeviceInterface,
      NULL,
      OnDevicesCreatedThunk,
      this);
}

void SysInfoNetwork::UpdateDeviceType(GVariant* value) {
  SendUpdate(g_variant_get_uint32(value));
}

void SysInfoNetwork::SendUpdate(guint new_device_type) {
  if (device_type_ == new_device_type)
    return;

  device_type_ = new_device_type;
  type_ = ToNetworkType(device_type_);

  picojson::value output = picojson::value(picojson::object());;
  picojson::value data = picojson::value(picojson::object());

  SetData(data);
  system_info::SetPicoJsonObjectValue(output, "cmd",
      picojson::value("SystemInfoPropertyValueChanged"));
  system_info::SetPicoJsonObjectValue(output, "prop",
      picojson::value("NETWORK"));
  system_info::SetPicoJsonObjectValue(output, "data", data);

  std::string result = output.serialize();
  api_->PostMessage(result.c_str());
}

void SysInfoNetwork::OnNetworkManagerSignal(GDBusProxy* proxy,
                                            gchar* sender,
                                            gchar* signal,
                                            GVariant* parameters,
                                            gpointer user_data) {
  SysInfoNetwork* self = reinterpret_cast<SysInfoNetwork*>(user_data);

  if (strcmp(signal, "PropertiesChanged") != 0)
    return;

  GVariantIter* iter;
  const gchar* key;
  GVariant* value;

  g_variant_get(parameters, "(a{sv})", &iter);
  while (g_variant_iter_loop(iter, "{&sv}", &key, &value)) {
    if (strcmp(key, "ActiveConnections") == 0) {
      self->UpdateActiveConnection(value);
      break;
    }
  }
  g_variant_iter_free(iter);
}

void SysInfoNetwork::OnActiveConnectionsSignal(GDBusProxy* proxy,
                                               gchar* sender,
                                               gchar* signal,
                                               GVariant* parameters,
                                               gpointer user_data) {
  SysInfoNetwork* self = reinterpret_cast<SysInfoNetwork*>(user_data);

  if (strcmp(signal, "PropertiesChanged") != 0)
    return;

  GVariantIter* iter;
  const gchar* key;
  GVariant* value;

  g_variant_get(parameters, "(a{sv})", &iter);
  while (g_variant_iter_loop(iter, "{&sv}", &key, &value)) {
    if (strcmp(key, "Devices") == 0) {
      self->UpdateActiveDevice(value);
      break;
    }
  }
  g_variant_iter_free(iter);
}

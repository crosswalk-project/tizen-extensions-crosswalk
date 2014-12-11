// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_wifi_network.h"

#include <NetworkManager.h>

#define NM_WIRELESS              NM_DBUS_INTERFACE_DEVICE ".Wireless"
#define NM_IP4_ADDRESS           NM_DBUS_INTERFACE_DEVICE ".Ip4Address"

namespace {

const double kWifiSignalStrengthDivisor = 100.0;

}  // namespace

SysInfoWifiNetwork::SysInfoWifiNetwork()
    : signal_strength_(0.0),
      ip_address_(""),
      ipv6_address_(""),
      ssid_(""),
      status_("OFF") {
  PlatformInitialize();
}

SysInfoWifiNetwork::~SysInfoWifiNetwork() {}

void SysInfoWifiNetwork::PlatformInitialize() {
  active_access_point_ = "";
  active_connection_ = "";
  active_device_ = "";
  device_type_ = NM_DEVICE_TYPE_UNKNOWN;
  ipv6_config_ = "";

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
      G_DBUS_PROXY_FLAGS_NONE,
      NULL,
      NM_DBUS_SERVICE,
      NM_DBUS_PATH,
      NM_DBUS_INTERFACE,
      NULL,
      OnNetworkManagerCreatedThunk,
      this);
}

void SysInfoWifiNetwork::StartListening() { }
void SysInfoWifiNetwork::StopListening() { }

void SysInfoWifiNetwork::SetData(picojson::value& data) {
  system_info::SetPicoJsonObjectValue(data, "status",
      picojson::value(status_));
  system_info::SetPicoJsonObjectValue(data, "ssid",
      picojson::value(ssid_));
  system_info::SetPicoJsonObjectValue(data, "ipAddress",
      picojson::value(IPAddressConverter(ip_address_desktop_)));
  system_info::SetPicoJsonObjectValue(data, "ipv6Address",
      picojson::value(ipv6_address_));
  system_info::SetPicoJsonObjectValue(data, "signalStrength",
      picojson::value(signal_strength_));
}

bool SysInfoWifiNetwork::Update(picojson::value& error) {
  // The type_ will be updated by NM signals.
  return true;
}

void SysInfoWifiNetwork::UpdateStatus(guint new_device_type) {
  if (device_type_ == new_device_type)
    return;

  device_type_ = new_device_type;
  if (device_type_ != NM_DEVICE_TYPE_WIFI) {
    status_ = "OFF";
    ssid_ = "";
    ip_address_desktop_ = 0;
    ipv6_address_ = "";
    signal_strength_ = 0.0;
    SendUpdate();
    return;
  }
  status_ = "ON";
  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
      G_DBUS_PROXY_FLAGS_NONE,
      NULL,
      NM_DBUS_SERVICE,
      active_device_.c_str(),
      NM_WIRELESS,
      NULL,
      OnActiveAccessPointCreatedThunk,
      this);
}

void SysInfoWifiNetwork::OnNetworkManagerCreated(GObject*, GAsyncResult* res) {
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

  // NetworkManager does not support g-properties-changed signal.
  g_signal_connect(proxy, "g-signal",
      G_CALLBACK(SysInfoWifiNetwork::OnNetworkManagerSignal), this);
}

void SysInfoWifiNetwork::OnAccessPointCreated(GObject*,
                                              GAsyncResult* res) {
  GError* err = 0;
  GDBusProxy* proxy = g_dbus_proxy_new_for_bus_finish(res, &err);

  if (!proxy) {
    g_printerr("AccessPoint proxy creation error: %s\n", err->message);
    g_error_free(err);
    return;
  }

  GVariant* ssid = g_dbus_proxy_get_cached_property(proxy, "Ssid");
  GVariant* strength = g_dbus_proxy_get_cached_property(proxy, "Strength");
  if (!ssid || !strength) {
    g_printerr("Failed to get ssid or strength.");
    return;
  }
  UpdateSSID(ssid);
  UpdateSignalStrength(strength);

  g_signal_connect(proxy, "g-signal",
      G_CALLBACK(SysInfoWifiNetwork::OnAccessPointSignal), this);
}

void SysInfoWifiNetwork::OnActiveAccessPointCreated(GObject*,
                                                    GAsyncResult* res) {
  GError* err = 0;
  GDBusProxy* proxy = g_dbus_proxy_new_for_bus_finish(res, &err);

  if (!proxy) {
    g_printerr("AccessPoint proxy creation error: %s\n", err->message);
    g_error_free(err);
    return;
  }

  GVariant* value = g_dbus_proxy_get_cached_property(proxy,
                                                     "ActiveAccessPoint");
  if (!value) {
    g_printerr("Failed to get activeaccesspoint.");
    return;
  }
  UpdateActiveAccessPoint(value);
}

void SysInfoWifiNetwork::OnActiveConnectionCreated(GObject*,
                                                   GAsyncResult* res) {
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
      G_CALLBACK(SysInfoWifiNetwork::OnActiveConnectionsSignal), this);
}

void SysInfoWifiNetwork::OnDevicesCreated(GObject*, GAsyncResult* res) {
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
  UpdateStatus(g_variant_get_uint32(value));
}

void SysInfoWifiNetwork::OnIPAddressCreated(GObject*, GAsyncResult* res) {
  GError* err = 0;
  GDBusProxy* proxy = g_dbus_proxy_new_for_bus_finish(res, &err);

  if (!proxy) {
    g_printerr("IPAddress proxy creation error: %s\n", err->message);
    g_error_free(err);
    return;
  }

  GVariant* value = g_dbus_proxy_get_cached_property(proxy, "Ip4Address");
  if (!value) {
    g_printerr("Failed to get Ip4Address.");
    return;
  }
  UpdateIPAddress(value);
}

void SysInfoWifiNetwork::OnIPv6AddressCreated(GObject*, GAsyncResult* res) {
  GError* err = 0;
  GDBusProxy* proxy = g_dbus_proxy_new_for_bus_finish(res, &err);

  if (!proxy) {
    g_printerr("IPv6Address proxy creation error: %s\n", err->message);
    g_error_free(err);
    return;
  }

  GVariant* value = g_dbus_proxy_get_cached_property(proxy, "Ip6Config");
  if (!value) {
    g_printerr("Failed to get Ip6Config.");
    return;
  }

  const char* str = g_variant_get_string(value, NULL);
  if (str && (strcmp(ipv6_config_.c_str(), str) != 0)) {
    if (strlen(str) > 2) {
      ipv6_config_ = std::string(str);
      g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
          G_DBUS_PROXY_FLAGS_NONE,
          NULL,
          NM_DBUS_SERVICE,
          ipv6_config_.c_str(),
          NM_DBUS_INTERFACE_IP6_CONFIG,
          NULL,
          OnUpdateIPv6AddressThunk,
          this);
      return;
    }
  }
  ipv6_address_ = "";
  SendUpdate();
}

void SysInfoWifiNetwork::OnUpdateIPv6Address(GObject*, GAsyncResult* res) {
  GError* err = 0;
  GDBusProxy* proxy = g_dbus_proxy_new_for_bus_finish(res, &err);

  if (!proxy) {
    g_printerr("IP6 Address proxy creation error: %s\n", err->message);
    g_error_free(err);
    return;
  }

  GVariant* value = g_dbus_proxy_get_cached_property(proxy, "Addresses");
  if (!value) {
    g_printerr("Get IP6 Addresses failed.");
    return;
  }
  UpdateIPv6Address(value);
}

void SysInfoWifiNetwork::UpdateActiveAccessPoint(GVariant* value) {
  const char* str = g_variant_get_string(value, NULL);
  if (str && (strcmp(active_access_point_.c_str(), str) != 0)) {
    active_access_point_ = std::string(str);
  }

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
      G_DBUS_PROXY_FLAGS_NONE,
      NULL,
      NM_DBUS_SERVICE,
      active_access_point_.c_str(),
      NM_DBUS_INTERFACE_ACCESS_POINT,
      NULL,
      OnAccessPointCreatedThunk,
      this);
}

void SysInfoWifiNetwork::UpdateActiveConnection(GVariant* value) {
  if (!value || !g_variant_n_children(value)) {
    active_connection_ = "";
    active_device_ = "";
    UpdateStatus(NM_DEVICE_TYPE_UNKNOWN);
    return;
  }

  GVariant* child = g_variant_get_child_value(value, 0);
  if (!child) {
    g_printerr("Invalid child[0] for ActiveConnections.");
    UpdateStatus(NM_DEVICE_TYPE_UNKNOWN);
    return;
  }

  const char* str = g_variant_get_string(child, NULL);
  if (str && (strcmp(active_connection_.c_str(), str) != 0)) {
    active_connection_ = std::string(str);
    active_device_ = "";
    UpdateStatus(NM_DEVICE_TYPE_UNKNOWN);
  }

  if (active_connection_.empty())
    return;

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
      G_DBUS_PROXY_FLAGS_NONE,
      NULL,
      NM_DBUS_SERVICE,
      active_connection_.c_str(),
      NM_DBUS_INTERFACE_ACTIVE_CONNECTION,
      NULL,
      OnActiveConnectionCreatedThunk,
      this);
}

void SysInfoWifiNetwork::UpdateActiveDevice(GVariant* value) {
  if (!value || !g_variant_n_children(value)) {
    active_device_ = "";
    UpdateStatus(NM_DEVICE_TYPE_UNKNOWN);
    return;
  }

  GVariant* child = g_variant_get_child_value(value, 0);
  if (!child) {
    g_printerr("Invalid child[0] for Devices.");
    UpdateStatus(NM_DEVICE_TYPE_UNKNOWN);
    return;
  }

  const char* str = g_variant_get_string(child, NULL);
  if (str && (strcmp(active_device_.c_str(), str) != 0)) {
    active_device_ = std::string(str);
    UpdateStatus(NM_DEVICE_TYPE_UNKNOWN);
  }

  if (active_device_.empty())
    return;

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
      G_DBUS_PROXY_FLAGS_NONE,
      NULL,
      NM_DBUS_SERVICE,
      active_device_.c_str(),
      NM_DBUS_INTERFACE_DEVICE,
      NULL,
      OnDevicesCreatedThunk,
      this);
}

void SysInfoWifiNetwork::UpdateIPAddress(GVariant* value) {
  // FIXME(guanxian): IP updates depend on others because of no signals.
  guint32 new_ip_address = g_variant_get_uint32(value);
  ip_address_desktop_ = new_ip_address;
  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
      G_DBUS_PROXY_FLAGS_NONE,
      NULL,
      NM_DBUS_SERVICE,
      active_device_.c_str(),
      NM_DBUS_INTERFACE_DEVICE,
      NULL,
      OnIPv6AddressCreatedThunk,
      this);
}

void SysInfoWifiNetwork::UpdateIPv6Address(GVariant* value) {
  // FIXME(guanxian): IPv6 updates depend on others because of no signals.
  GVariant* child_group = g_variant_get_child_value(value, 0);
  GVariant* child = g_variant_get_child_value(child_group, 0);
  gsize length = g_variant_get_size(child);
  gconstpointer g_pointer = g_variant_get_fixed_array(child, &length,
                                                      sizeof(guchar));
  unsigned char* addr = NULL;
  addr = static_cast<unsigned char*>(const_cast<void*>(g_pointer));
  char c_addr1[3];
  char c_addr2[3];
  std::string s_addr1;
  std::string s_addr2;
  snprintf(c_addr1, sizeof(c_addr1), "%.2x", static_cast<int>(*(addr++)));
  s_addr1 = std::string(c_addr1);
  snprintf(c_addr2, sizeof(c_addr2), "%.2x", static_cast<int>(*(addr++)));
  s_addr2 = std::string(c_addr2);
  ipv6_address_ = s_addr1 + s_addr2;

  for (int i = 2; i < length; i += 2) {
    snprintf(c_addr1, sizeof(c_addr1), "%.2x", static_cast<int>(*(addr++)));
    s_addr1 = std::string(c_addr1);
    snprintf(c_addr2, sizeof(c_addr2), "%.2x", static_cast<int>(*(addr++)));
    s_addr2 = std::string(c_addr2);
    ipv6_address_ += ":" + s_addr1 + s_addr2;
  }

  SendUpdate();
}

void SysInfoWifiNetwork::UpdateSSID(GVariant* value) {
  gsize length = g_variant_get_size(value);
  gconstpointer g_pointer = g_variant_get_fixed_array(value, &length,
                                                      sizeof(guchar));
  char* ssid = new char[length + 1];
  strncpy(ssid, reinterpret_cast<char*>(const_cast<void*>(g_pointer)), length);
  ssid[length] = '\0';
  ssid_ = (ssid_ == std::string(ssid)) ? ssid_ : std::string(ssid);
  delete[] ssid;
}

void SysInfoWifiNetwork::UpdateSignalStrength(GVariant* value) {
  int result = static_cast<int>(g_variant_get_byte(value));
  double strength = static_cast<double>(result) / kWifiSignalStrengthDivisor;
  if (strength == signal_strength_)
    return;
  signal_strength_ = strength;
  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
      G_DBUS_PROXY_FLAGS_NONE,
      NULL,
      NM_DBUS_SERVICE,
      active_device_.c_str(),
      NM_DBUS_INTERFACE_DEVICE,
      NULL,
      OnIPAddressCreatedThunk,
      this);
}

void SysInfoWifiNetwork::OnAccessPointSignal(GDBusProxy* proxy,
                                             gchar* sender,
                                             gchar* signal,
                                             GVariant* parameters,
                                             gpointer user_data) {
  SysInfoWifiNetwork* self = reinterpret_cast<SysInfoWifiNetwork*>(user_data);

  if (strcmp(signal, "PropertiesChanged") != 0)
    return;

  GVariantIter* iter;
  const gchar* key;
  GVariant* value;

  g_variant_get(parameters, "(a{sv})", &iter);
  while (g_variant_iter_loop(iter, "{&sv}", &key, &value)) {
    if (strcmp(key, "Ssid") == 0) {
      self->UpdateSSID(value);
    } else if (strcmp(key, "Strength") == 0) {
      self->UpdateSignalStrength(value);
    }
  }
  g_variant_iter_free(iter);
}

void SysInfoWifiNetwork::OnNetworkManagerSignal(GDBusProxy* proxy,
                                                gchar* sender,
                                                gchar* signal,
                                                GVariant* parameters,
                                                gpointer user_data) {
  SysInfoWifiNetwork* self = reinterpret_cast<SysInfoWifiNetwork*>(user_data);

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

void SysInfoWifiNetwork::OnActiveConnectionsSignal(GDBusProxy* proxy,
                                                   gchar* sender,
                                                   gchar* signal,
                                                   GVariant* parameters,
                                                   gpointer user_data) {
  SysInfoWifiNetwork* self = reinterpret_cast<SysInfoWifiNetwork*>(user_data);

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

std::string SysInfoWifiNetwork::IPAddressConverter(unsigned int ip) {
  char c_ip_b1[4];
  char c_ip_b2[4];
  char c_ip_b3[4];
  char c_ip_b4[4];
  unsigned int divisor = 256;

  if (ip < 0 || ip > INT_MAX)
    return "";

  snprintf(c_ip_b1, sizeof(c_ip_b1), "%d", ip % divisor);
  ip /= divisor;
  snprintf(c_ip_b2, sizeof(c_ip_b2), "%d", ip % divisor);
  ip /= divisor;
  snprintf(c_ip_b3, sizeof(c_ip_b3), "%d", ip % divisor);
  ip /= divisor;
  snprintf(c_ip_b4, sizeof(c_ip_b4), "%d", ip % divisor);

  std::string s_ip_b1(c_ip_b1);
  std::string s_ip_b2(c_ip_b2);
  std::string s_ip_b3(c_ip_b3);
  std::string s_ip_b4(c_ip_b4);

  ip_address_ = s_ip_b1 + "." + s_ip_b2 + "." + s_ip_b3 + "." + s_ip_b4;
  return ip_address_;
}

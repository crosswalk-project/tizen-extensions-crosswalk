// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bluetooth/bluetooth_context.h"
#include "common/picojson.h"

static void getPropertyValue(const char* key, GVariant* value,
    picojson::value::object& o) {
  const std::string key_str(key);
  if (key_str == "Class") {
    guint32 class_id = g_variant_get_uint32(value);
    o[key_str] = picojson::value(static_cast<double>(class_id));
  } else if (key_str == "RSSI") {
    gint16 class_id = g_variant_get_int16(value);
    o[key_str] = picojson::value(static_cast<double>(class_id));
  } else if (key_str != "Devices") {
    char* value_str = g_variant_print(value, FALSE);
    o[key_str] = picojson::value(value_str);
    g_free(value_str);
  }
}

void BluetoothContext::OnSignal(GDBusProxy* proxy, gchar* sender_name, gchar* signal,
      GVariant* parameters, gpointer user_data) {

  BluetoothContext* handler = reinterpret_cast<BluetoothContext*>(user_data);

  std::string s(signal);
  if (s == "DeviceFound") {
    char* address;
    GVariantIter* iter;

    g_variant_get(parameters, "(sa{sv})", &address, &iter);
    handler->DeviceFound(std::string(address), iter);
  }
}

void BluetoothContext::OnGotAdapterProperties(GObject*, GAsyncResult* res) {
  GError* error = NULL;
  GVariant* result = g_dbus_proxy_call_finish(adapter_proxy_, res, &error);

  if (result == NULL) {
    g_printerr("\n\nError Got DefaultAdapter Properties: %s\n", error->message);
    g_error_free(error);
    return;
  }

  const gchar* key;
  GVariant* value;
  GVariantIter* iter;
  g_variant_get(result, "(a{sv})", &iter);

  while (g_variant_iter_loop(iter, "{sv}", &key, &value)) {
    const std::string key_str(key);
    if (key_str != "Devices")
      adapter_info_[key] = g_variant_print(value, FALSE);
  }
  g_variant_iter_free(iter);
}

void BluetoothContext::OnAdapterProxyCreated(GObject*, GAsyncResult* res) {
  GError* error = NULL;
  adapter_proxy_ = g_dbus_proxy_new_for_bus_finish(res, &error);

  if (adapter_proxy_ == NULL) {
    g_printerr("\n\n## adapter_proxy_ creation error: %s\n", error->message);
    g_error_free(error);
    return;
  }

  g_dbus_proxy_call(adapter_proxy_, "GetProperties", NULL,
    G_DBUS_CALL_FLAGS_NONE, 5000, NULL, OnGotAdapterPropertiesThunk, this);

  g_signal_connect(adapter_proxy_, "g-signal",
      G_CALLBACK(BluetoothContext::OnSignal), this);
}

void BluetoothContext::OnManagerCreated(GObject*, GAsyncResult* res) {
  GError* err = NULL;
  manager_proxy_ = g_dbus_proxy_new_for_bus_finish(res, &err);

  if (manager_proxy_ == NULL) {
    g_printerr("## Manager Proxy creation error: %s\n", err->message);
    g_error_free(err);
    return;
  }

  g_dbus_proxy_call(manager_proxy_, "DefaultAdapter", NULL,
      G_DBUS_CALL_FLAGS_NONE, 5000, NULL, OnGotDefaultAdapterPathThunk, this);
}

void BluetoothContext::OnGotDefaultAdapterPath(GObject*, GAsyncResult* res) {
  GError* error = NULL;
  GVariant* result = g_dbus_proxy_call_finish(manager_proxy_, res, &error);

  if (result == NULL) {
    g_printerr("\n\nError Got DefaultAdapter Path: %s\n", error->message);
    g_error_free(error);
    return;
  }

  char* path;
  g_variant_get(result, "(o)", &path);

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
      G_DBUS_PROXY_FLAGS_NONE,
      NULL, /* GDBusInterfaceInfo */
      "org.bluez",
      path,
      "org.bluez.Adapter",
      NULL, /* GCancellable */
      OnAdapterProxyCreatedThunk,
      this);

  g_variant_unref(result);
  g_free(path);
}

BluetoothContext::~BluetoothContext() {
  delete api_;

  if (adapter_proxy_ != NULL)
    g_object_unref(adapter_proxy_);

  DeviceMap::iterator it;
  for (it = found_devices_.begin(); it != found_devices_.end(); ++it)
    g_variant_iter_free(it->second);
}

void BluetoothContext::PlatformInitialize() {
  adapter_proxy_ = NULL;
  manager_proxy_ = NULL;
  is_js_context_initialized_ = false;

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
      G_DBUS_PROXY_FLAGS_NONE,
      NULL, /* GDBusInterfaceInfo */
      "org.bluez",
      "/",
      "org.bluez.Manager",
      NULL, /* GCancellable */
      OnManagerCreatedThunk,
      this);
}

picojson::value BluetoothContext::HandleGetDefaultAdapter(const picojson::value& msg) {
  if (adapter_info_.empty())
    return picojson::value();

  picojson::value::object o;
  o["cmd"] = picojson::value("");
  o["reply_id"] = picojson::value(msg.get("reply_id").to_str());

  o["name"] = picojson::value(adapter_info_["Name"]);
  o["address"] = picojson::value(adapter_info_["Address"]);

  bool powered = (adapter_info_["Powered"] == "true") ? true : false;
  o["powered"] = picojson::value(powered);

  bool visible = (adapter_info_["Visible"] == "true") ? true : false;
  o["visible"] = picojson::value(visible);

  // This is the JS API entry point, so we should clean our message queue
  // on the next PostMessage call.
  if (!is_js_context_initialized_)
    is_js_context_initialized_ = true;

  picojson::value v(o);
  return v;
}

void BluetoothContext::DeviceFound(std::string address, GVariantIter* properties) {
  const gchar* key;
  GVariant* value;
  picojson::value::object o;
  DeviceMap::iterator it = found_devices_.find(address);

  if (it == found_devices_.end()) { // Found on discovery.
    o["cmd"] = picojson::value("DeviceFound");
    o["found_on_discovery"] = picojson::value(true);

    while (g_variant_iter_loop(properties, "{sv}", &key, &value))
      getPropertyValue(key, value, o);

    found_devices_[address] = properties;
  } else { // Updated during discovery.
    o["cmd"] = picojson::value("DeviceUpdated");
    o["found_on_discovery"] = picojson::value(false);

    while (g_variant_iter_loop(properties, "{sv}", &key, &value))
      getPropertyValue(key, value, o);

    found_devices_[address] = properties;
  }

  picojson::value v(o);
  PostMessage(v);
}

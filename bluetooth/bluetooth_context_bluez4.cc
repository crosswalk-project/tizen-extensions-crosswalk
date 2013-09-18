// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bluetooth/bluetooth_context.h"
#include "common/picojson.h"

#if defined(TIZEN_MOBILE)
#include <bluetooth.h>
#endif

static void getPropertyValue(const char* key, GVariant* value,
    picojson::value::object& o) {
  if (!strcmp(key, "Class")) {
    guint32 class_id = g_variant_get_uint32(value);
    o[key] = picojson::value(static_cast<double>(class_id));
  } else if (!strcmp(key, "RSSI")) {
    gint16 class_id = g_variant_get_int16(value);
    o[key] = picojson::value(static_cast<double>(class_id));
  } else if (strcmp(key, "Devices")) { // FIXME(jeez): Handle 'Devices' property.
    std::string value_str;
    if (g_variant_is_of_type(value, G_VARIANT_TYPE_STRING))
      value_str = g_variant_get_string(value, NULL);
    else
      value_str = g_variant_print(value, false);
    o[key] = picojson::value(value_str);
  }
}

void BluetoothContext::OnSignal(GDBusProxy* proxy, gchar* sender, gchar* signal,
      GVariant* parameters, gpointer data) {
  BluetoothContext* handler = reinterpret_cast<BluetoothContext*>(data);

  if (!strcmp(signal, "DeviceFound")) {
    char* address;
    GVariantIter* it;

    g_variant_get(parameters, "(sa{sv})", &address, &it);
    handler->DeviceFound(std::string(address), it);
  } else if (!strcmp(signal, "PropertyChanged")) {
    char* name;
    GVariant* value;

    g_variant_get(parameters, "(sv)", &name, &value);

    if (!strcmp(name, "Devices")) {
      char* path;
      GVariantIter *iter;

      g_variant_get(value, "ao", &iter);

      while (g_variant_iter_loop(iter, "o", &path)) {
        g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
                                 NULL,
                                 /* GDBusInterfaceInfo */
                                 "org.bluez", path, "org.bluez.Device",
                                 NULL,
                                 /* GCancellable */
                                 OnDeviceProxyCreatedThunk, data);
      }

      g_variant_iter_free(iter);
    } else {
      picojson::value::object property_updated;
      property_updated["cmd"] = picojson::value("AdapterUpdated");
      property_updated[name] = picojson::value(handler->adapter_info_[name]);
      handler->PostMessage(picojson::value(property_updated));

      // If in our callback ids map we have a reply_id related to the property
      // being updated now, then we must also reply to the PostMessage call.
      // This way we enforce that our JavaScript context calls the onsuccess
      // return callback only after the property has actually been modified.
      std::map<std::string, std::string>::iterator it =
          handler->callbacks_map_.find(name);

      if (it != handler->callbacks_map_.end()) {
        picojson::value::object property_changed;
        property_changed["cmd"] = picojson::value("");
        property_changed["reply_id"] = picojson::value(it->second);
        property_changed["error"] = picojson::value(static_cast<double>(0));
        handler->PostMessage(picojson::value(property_changed));
        handler->callbacks_map_.erase(it);
      }

      g_variant_unref(value);
    }
  }
}

void BluetoothContext::OnDeviceSignal(GDBusProxy* proxy, gchar* sender, gchar* signal,
      GVariant* parameters, gpointer data) {
  BluetoothContext* handler = reinterpret_cast<BluetoothContext*>(data);
  const char* iface = g_dbus_proxy_get_interface_name(proxy);

  // We only want org.bluez.Device signals.
  if (strcmp(iface, "org.bluez.Device"))
    return;

  // More specifically, PropertyChanged ones.
  if (strcmp(signal, "PropertyChanged"))
    return;

  const char* path = g_dbus_proxy_get_object_path(proxy);

  std::map<std::string, std::string>::iterator it =
      handler->object_path_address_map_.find(path);
  if (it == handler->object_path_address_map_.end())
    return;

  const char *address = it->second.c_str();

  const gchar* key;
  GVariant* value;
  picojson::value::object o;

  o["cmd"] = picojson::value("DeviceUpdated");
  o["found_on_discovery"] = picojson::value(false);
  o["Address"] = picojson::value(address);

  g_variant_get(parameters, "(sv)", &key, &value);

  getPropertyValue(key, value, o);

  handler->PostMessage(picojson::value(o));
}

void BluetoothContext::OnGotAdapterProperties(GObject*, GAsyncResult* res) {
  GError* error = 0;
  GVariant* result = g_dbus_proxy_call_finish(adapter_proxy_, res, &error);

  if (!result) {
    g_printerr("\n\nError Got DefaultAdapter Properties: %s\n", error->message);
    g_error_free(error);
    return;
  }

  const gchar* key;
  GVariant* value;
  GVariantIter* it;
  g_variant_get(result, "(a{sv})", &it);

  while (g_variant_iter_loop(it, "{sv}", &key, &value)) {
    if (!strcmp(key, "Devices")) {
      char* path;
      GVariantIter *iter;

      g_variant_get(value, "ao", &iter);

      while (g_variant_iter_loop(iter, "o", &path)) {
        DeviceMap::iterator it = known_devices_.find(path);
        if (it != known_devices_.end())
          continue;

        g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
                                 NULL,
                                 /* GDBusInterfaceInfo */
                                 "org.bluez", path, "org.bluez.Device",
                                 NULL,
                                 /* GCancellable */
                                 OnDeviceProxyCreatedThunk, this);
      }

      g_variant_iter_free(iter);
    } else {
      if (g_variant_is_of_type(value, G_VARIANT_TYPE_STRING))
        adapter_info_[key] = std::string(g_variant_get_string(value, NULL));
      else
        adapter_info_[key] = g_variant_print(value, false);
    }
  }

  g_variant_iter_free(it);
}

void BluetoothContext::OnAdapterPropertySet(std::string property, GAsyncResult* res) {
  GError* error = 0;
  GVariant* result = g_dbus_proxy_call_finish(adapter_proxy_, res, &error);

  // We should only reply to the PostMessage here if an error happened when
  // changing the property. For replying to the successful property change
  // we wait until BluetoothContext::OnSignal receives the related PropertyChange
  // signal, so we avoid that our JavaScript context calls the onsuccess return
  // callback before the property was actually updated on the adapter.
  if (!result) {
    g_printerr("\n\nError Got DefaultAdapter Property SET: %s\n", error->message);
    g_error_free(error);
    picojson::value::object o;
    o["cmd"] = picojson::value("");
    o["reply_id"] = picojson::value(callbacks_map_[property]);

    // No matter the error info here, BlueZ4's documentation says the only
    // error that can be raised here is org.bluez.Error.InvalidArguments.
    o["error"] = picojson::value(static_cast<double>(1));
    PostMessage(picojson::value(o));

    callbacks_map_.erase(property);
    return;
  }

  g_variant_unref(result);
}

void BluetoothContext::OnAdapterProxyCreated(GObject*, GAsyncResult* res) {
  GError* error = 0;
  adapter_proxy_ = g_dbus_proxy_new_for_bus_finish(res, &error);

  if (!adapter_proxy_) {
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
  GError* err = 0;
  manager_proxy_ = g_dbus_proxy_new_for_bus_finish(res, &err);

  if (!manager_proxy_) {
    g_printerr("## Manager Proxy creation error: %s\n", err->message);
    g_error_free(err);
    return;
  }

  g_dbus_proxy_call(manager_proxy_, "DefaultAdapter", NULL,
      G_DBUS_CALL_FLAGS_NONE, 5000, NULL, OnGotDefaultAdapterPathThunk, this);
}

void BluetoothContext::OnGotDefaultAdapterPath(GObject*, GAsyncResult* res) {
  GError* error = 0;
  GVariant* result = g_dbus_proxy_call_finish(manager_proxy_, res, &error);

  if (!result) {
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

void BluetoothContext::OnAdapterCreateBonding(GObject*, GAsyncResult* res) {
  GError* error = 0;
  GVariant* result = g_dbus_proxy_call_finish(adapter_proxy_, res, &error);

  picojson::value::object o;
  o["cmd"] = picojson::value("");
  o["reply_id"] = picojson::value(callbacks_map_["CreateBonding"]);
  o["error"] = picojson::value(static_cast<double>(0));

  if (!result) {
    g_printerr("\n\nError on creating adapter bonding: %s\n", error->message);
    g_error_free(error);

    o["error"] = picojson::value(static_cast<double>(1));
  } else {
    g_variant_unref(result);
  }

  PostMessage(picojson::value(o));
  callbacks_map_.erase("CreateBonding");
}

void BluetoothContext::OnAdapterDestroyBonding(GObject*, GAsyncResult* res) {
  GError* error = 0;
  GVariant* result = g_dbus_proxy_call_finish(adapter_proxy_, res, &error);

  picojson::value::object o;
  o["cmd"] = picojson::value("");
  o["reply_id"] = picojson::value(callbacks_map_["DestroyBonding"]);
  o["error"] = picojson::value(static_cast<double>(0));

  if (!result) {
    g_printerr("\n\nError on destroying adapter bonding: %s\n", error->message);
    g_error_free(error);

    o["error"] = picojson::value(static_cast<double>(2));
  } else {
    g_variant_unref(result);
  }

  PostMessage(picojson::value(o));
  callbacks_map_.erase("DestroyBonding");
}

void BluetoothContext::OnFoundDevice(GObject*, GAsyncResult* res) {
  picojson::value::object o;
  char* object_path;
  GError* error = 0;
  GVariant* result = g_dbus_proxy_call_finish(adapter_proxy_, res, &error);

  if (!result) {
    g_printerr("\n\nError on destroying adapter bonding: %s\n", error->message);
    g_error_free(error);

    o["cmd"] = picojson::value("");
    o["reply_id"] = picojson::value(callbacks_map_["DestroyBonding"]);
    o["error"] = picojson::value(static_cast<double>(1));

    PostMessage(picojson::value(o));
    callbacks_map_.erase("DestroyBonding");
    return;
  }

  g_variant_get(result, "(o)", &object_path);
  g_dbus_proxy_call(adapter_proxy_, "RemoveDevice",
      g_variant_new("(o)", object_path),
      G_DBUS_CALL_FLAGS_NONE, -1, NULL, OnAdapterDestroyBondingThunk, this);

  g_variant_unref(result);
}

BluetoothContext::~BluetoothContext() {
  delete api_;

  if (adapter_proxy_)
    g_object_unref(adapter_proxy_);

  DeviceMap::iterator it;
  for (it = known_devices_.begin(); it != known_devices_.end(); ++it)
    g_object_unref(it->second);

#if defined(TIZEN_MOBILE)
    bt_deinitialize();
#endif
}

void BluetoothContext::PlatformInitialize() {
  adapter_proxy_ = 0;
  manager_proxy_ = 0;
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
  picojson::value::object o;

  // If the bluetooth daemon is off, then manager_proxy_ is NULL.
  // Hence, adapter_proxy_ is also NULL and adapter_info_ won't
  // have been filled. If the daemon is running but the
  // adapter is off (i.e.: hci0 down), then adapter_info_ will
  // have been filled and adapter_info_["Powered"] is false.
  if (adapter_info_.empty()) {
    o["cmd"] = picojson::value("");
    o["reply_id"] = picojson::value(msg.get("reply_id").to_str());
    o["error"] = picojson::value(static_cast<double>(1));
    return picojson::value(o);
  }

  o["cmd"] = picojson::value("");
  o["reply_id"] = picojson::value(msg.get("reply_id").to_str());

  o["name"] = picojson::value(adapter_info_["Name"]);
  o["address"] = picojson::value(adapter_info_["Address"]);

  bool powered = (adapter_info_["Powered"] == "true") ? true : false;
  o["powered"] = picojson::value(powered);

  bool visible = (adapter_info_["Discoverable"] == "true") ? true : false;
  o["visible"] = picojson::value(visible);

  o["error"] = picojson::value(static_cast<double>(0));

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

  o["cmd"] = picojson::value("DeviceFound");
  o["found_on_discovery"] = picojson::value(true);

  while (g_variant_iter_loop(properties, "{sv}", &key, &value))
    getPropertyValue(key, value, o);

  picojson::value v(o);
  PostMessage(v);
}

void BluetoothContext::HandleSetAdapterProperty(const picojson::value& msg) {
  std::string property = msg.get("property").to_str();

  GVariant* value = 0;
  if (property == "Name")
    value = g_variant_new("s", msg.get("value").to_str().c_str());
  else if (property == "Discoverable") {
    value = g_variant_new("b", msg.get("value").get<bool>());

    if (msg.contains("timeout")) {
      const guint32 timeout = static_cast<guint32>(msg.get("timeout").get<double>());
      g_dbus_proxy_call(adapter_proxy_, "SetProperty",
          g_variant_new("(sv)", "DiscoverableTimeout", g_variant_new("u", timeout)),
          G_DBUS_CALL_FLAGS_NONE, 5000, NULL, NULL, NULL);
    }
  } else if (property == "Powered")
    value = g_variant_new("b", msg.get("value").get<bool>());

  assert(value);

  callbacks_map_[property] = msg.get("reply_id").to_str();

  OnAdapterPropertySetData* property_set_callback_data_ =
      new OnAdapterPropertySetData;
  property_set_callback_data_->property = property;
  property_set_callback_data_->bt_context = this;

  g_dbus_proxy_call(adapter_proxy_, "SetProperty",
      g_variant_new("(sv)", property.c_str(), value),
      G_DBUS_CALL_FLAGS_NONE, 5000, NULL, OnAdapterPropertySetThunk,
      property_set_callback_data_);
}

void BluetoothContext::HandleCreateBonding(const picojson::value& msg) {
  std::string address = msg.get("address").to_str();
  callbacks_map_["CreateBonding"] = msg.get("reply_id").to_str();

  g_dbus_proxy_call(adapter_proxy_, "CreatePairedDevice",
      g_variant_new ("(sos)", address.c_str(), "/", "KeyboardDisplay"),
      G_DBUS_CALL_FLAGS_NONE, -1, NULL, OnAdapterCreateBondingThunk, this);
}

void BluetoothContext::HandleDestroyBonding(const picojson::value& msg) {
  std::string address = msg.get("address").to_str();
  callbacks_map_["DestroyBonding"] = msg.get("reply_id").to_str();

  g_dbus_proxy_call(adapter_proxy_, "FindDevice",
      g_variant_new("(s)", address.c_str()),
      G_DBUS_CALL_FLAGS_NONE, -1, NULL, OnFoundDeviceThunk, this);
}

void BluetoothContext::OnDeviceProxyCreated(GObject* object, GAsyncResult* res) {
  GDBusProxy* device_proxy;
  GError* error = 0;

  device_proxy = g_dbus_proxy_new_for_bus_finish(res, &error);
  if (!device_proxy) {
    g_printerr("\n\n## device_proxy creation error: %s\n", error->message);
    g_error_free(error);
    return;
  }

  const char* path = g_dbus_proxy_get_object_path(device_proxy);
  known_devices_[path] = device_proxy;

  g_dbus_proxy_call(device_proxy, "GetProperties", NULL,
    G_DBUS_CALL_FLAGS_NONE, 5000, NULL, OnGotDevicePropertiesThunk, this);

  g_signal_connect(device_proxy, "g-signal",
    G_CALLBACK(BluetoothContext::OnDeviceSignal), this);
}

void BluetoothContext::OnGotDeviceProperties(GObject* object, GAsyncResult* res) {
  GError* error = 0;
  GDBusProxy *device_proxy = reinterpret_cast<GDBusProxy*>(object);
  GVariant* result = g_dbus_proxy_call_finish(device_proxy, res, &error);

  if (!result) {
    g_printerr("\n\nError OnGotDeviceProperties: %s\n", error->message);
    g_error_free(error);
    return;
  }

  const gchar* key;
  GVariant* value;
  GVariantIter* it;
  picojson::value::object o;

  o["cmd"] = picojson::value("DeviceUpdated");
  o["found_on_discovery"] = picojson::value(false);

  g_variant_get(result, "(a{sv})", &it);

  while (g_variant_iter_loop(it, "{sv}", &key, &value)) {
    if (!strcmp(key, "Address")) {
      const char* address = g_variant_get_string(value, NULL);
      const char* path = g_dbus_proxy_get_object_path(device_proxy);

      object_path_address_map_[path] = address;
    }

    getPropertyValue(key, value, o);

  }

  picojson::value v(o);
  PostMessage(v);
}

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bluetooth/bluetooth_context.h"
#include "common/picojson.h"

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

    // FIXME(jeez): Handle 'Devices' property.
    if (strcmp(name, "Devices")) {
      if (g_variant_is_of_type(value, G_VARIANT_TYPE_STRING)) {
        handler->adapter_info_[name] = std::string(g_variant_get_string(value,
            NULL));
      } else {
        handler->adapter_info_[name] = g_variant_print(value, false);
      }

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
    if (strcmp(key, "Devices")) { // FIXME(jeez): Handle 'Devices' property.
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

BluetoothContext::~BluetoothContext() {
  delete api_;

  if (adapter_proxy_)
    g_object_unref(adapter_proxy_);

  DeviceMap::iterator it;
  for (it = known_devices_.begin(); it != known_devices_.end(); ++it)
    g_variant_iter_free(it->second);
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
  if (adapter_info_.empty())
    return picojson::value();

  picojson::value::object o;
  o["cmd"] = picojson::value("");
  o["reply_id"] = picojson::value(msg.get("reply_id").to_str());

  o["name"] = picojson::value(adapter_info_["Name"]);
  o["address"] = picojson::value(adapter_info_["Address"]);

  bool powered = (adapter_info_["Powered"] == "true") ? true : false;
  o["powered"] = picojson::value(powered);

  bool visible = (adapter_info_["Discoverable"] == "true") ? true : false;
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
  DeviceMap::iterator it = known_devices_.find(address);

  if (it == known_devices_.end()) { // Found on discovery.
    o["cmd"] = picojson::value("DeviceFound");
    o["found_on_discovery"] = picojson::value(true);

    while (g_variant_iter_loop(properties, "{sv}", &key, &value))
      getPropertyValue(key, value, o);

    known_devices_[address] = properties;
  } else { // Updated during discovery.
    o["cmd"] = picojson::value("DeviceUpdated");
    o["found_on_discovery"] = picojson::value(false);

    while (g_variant_iter_loop(properties, "{sv}", &key, &value))
      getPropertyValue(key, value, o);

    known_devices_[address] = properties;
  }

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

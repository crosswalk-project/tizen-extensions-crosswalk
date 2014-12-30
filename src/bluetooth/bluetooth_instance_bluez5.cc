// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bluetooth/bluetooth_instance.h"

static void getPropertyValue(const char* key, GVariant* value,
    picojson::value::object& o) {
  if (!strcmp(key, "Class")) {
    guint32 class_id = g_variant_get_uint32(value);
    o["ClassMajor"] = picojson::value \
                      (static_cast<double>((class_id & 0x00001F00) >> 8));
    o["ClassMinor"] = picojson::value \
                      (static_cast<double>(class_id & 0x000000FC));
    o["ClassService"] = picojson::value\
                        (static_cast<double>(class_id & 0x00FF0000));
  } else if (!strcmp(key, "RSSI")) {
    gint16 class_id = g_variant_get_int16(value);
    o[key] = picojson::value(static_cast<double>(class_id));
  } else {
    char* value_str = g_variant_print(value, true);
    o[key] = picojson::value(value_str);
    g_free(value_str);
  }
}

void BluetoothInstance::OnPropertiesChanged(GDBusProxy* proxy,
    GVariant* changed_properties, const gchar* const* invalidated_properties,
    gpointer data) {
  if (!g_variant_n_children(changed_properties))
    return;

  const char* interface = g_dbus_proxy_get_interface_name(proxy);
  BluetoothInstance* handler = reinterpret_cast<BluetoothInstance*>(data);

  GVariantIter* iter;
  const gchar* key;
  GVariant* value;
  picojson::value::object o;

  g_variant_get(changed_properties, "a{sv}", &iter);

  if (!strcmp(interface, "org.bluez.Device1")) {
    DeviceMap::iterator it =
        handler->known_devices_.find(g_dbus_proxy_get_object_path(proxy));

    if (it == handler->known_devices_.end())
      return;

    o["cmd"] = picojson::value("DeviceUpdated");

    while (g_variant_iter_loop(iter, "{&sv}", &key, &value))
      getPropertyValue(key, value, o);

    GVariant* addr = g_dbus_proxy_get_cached_property(it->second, "Address");
    char* str = g_variant_print(addr, true);
    o["Address"] = picojson::value(str);
    g_free(str);
    g_variant_unref(addr);
  } else if (!strcmp(interface, "org.bluez.Adapter1")) {
    o["cmd"] = picojson::value("AdapterUpdated");

    while (g_variant_iter_loop(iter, "{&sv}", &key, &value)) {
      getPropertyValue(key, value, o);
      handler->adapter_info_[key] = o[key].get<std::string>();
    }
  }
  g_variant_iter_free(iter);
  picojson::value v(o);
  handler->InternalPostMessage(v);
}

void BluetoothInstance::OnDBusObjectAdded(GDBusObjectManager* manager,
    GDBusObject* object) {
  GDBusInterface* interface = g_dbus_object_get_interface(object,
      "org.bluez.Device1");
  if (interface) {
    g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
      NULL /* GDBusInterfaceInfo */, "org.bluez",
      g_dbus_object_get_object_path(object), "org.bluez.Device1",
      NULL /* GCancellable */, DeviceFoundThunk, this);
    g_object_unref(interface);
  }
}

void BluetoothInstance::OnDBusObjectRemoved(GDBusObjectManager* manager,
    GDBusObject* object) {
  GDBusInterface* interface = g_dbus_object_get_interface(object,
      "org.bluez.Device1");

  if (interface) {
    DeviceRemoved(object);
    g_object_unref(interface);
  }
}

void BluetoothInstance::OnAdapterProxyCreated(GObject*, GAsyncResult* res) {
  GError* error = 0;
  adapter_proxy_ = g_dbus_proxy_new_for_bus_finish(res, &error);

  if (!adapter_proxy_) {
    g_printerr("## adapter_proxy_ creation error: %s\n", error->message);
    g_error_free(error);
    return;
  }

  gchar** properties = g_dbus_proxy_get_cached_property_names(adapter_proxy_);

  for (unsigned int n = 0; properties && properties[n]; n++) {
    const gchar* key = properties[n];
    GVariant* value = g_dbus_proxy_get_cached_property(adapter_proxy_, key);
    gchar* value_str = g_variant_print(value, true);

    adapter_info_[key] = value_str;
    g_variant_unref(value);
    g_free(value_str);
  }

  g_signal_connect(adapter_proxy_, "g-properties-changed",
      G_CALLBACK(BluetoothInstance::OnPropertiesChanged), this);

  g_strfreev(properties);
}

void BluetoothInstance::CacheManagedObject(gpointer data, gpointer user_data) {
  GDBusObject* object = static_cast<GDBusObject*>(data);
  GDBusInterface* interface = g_dbus_object_get_interface(object,
      "org.bluez.Device1");

  if (interface) {
    g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
        NULL /* GDBusInterfaceInfo */, "org.bluez",
        g_dbus_object_get_object_path(object), "org.bluez.Device1",
        NULL /* GCancellable */, BluetoothInstance::KnownDeviceFoundThunk,
        user_data);

    g_object_unref(interface);
  }
}

void BluetoothInstance::OnManagerCreated(GObject*, GAsyncResult* res) {
  GError* err = 0;
  object_manager_ = g_dbus_object_manager_client_new_for_bus_finish(res, &err);

  if (!object_manager_) {
    g_printerr("## ObjectManager creation error: %s\n", err->message);
    g_error_free(err);
  } else {
    g_signal_connect(object_manager_, "object-added",
        G_CALLBACK(OnDBusObjectAddedThunk), this);
    g_signal_connect(object_manager_, "object-removed",
        G_CALLBACK(OnDBusObjectRemovedThunk), this);

    GList* managed_objects = g_dbus_object_manager_get_objects(object_manager_);
    g_list_foreach(managed_objects, CacheManagedObject, this);
    g_list_free(managed_objects);
  }
}

BluetoothInstance::~BluetoothInstance() {
  if (adapter_proxy_)
    g_object_unref(adapter_proxy_);
  if (object_manager_)
    g_object_unref(object_manager_);

  DeviceMap::iterator it;
  for (it = known_devices_.begin(); it != known_devices_.end(); ++it)
    g_object_unref(it->second);
}

void BluetoothInstance::PlatformInitialize() {
  adapter_proxy_ = 0;
  object_manager_ = 0;
  is_js_context_initialized_ = false;

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
      G_DBUS_PROXY_FLAGS_NONE,
      NULL, /* GDBusInterfaceInfo */
      "org.bluez",
      "/org/bluez/hci0",
      "org.bluez.Adapter1",
      NULL, /* GCancellable */
      OnAdapterProxyCreatedThunk,
      this);

  g_dbus_object_manager_client_new_for_bus(G_BUS_TYPE_SYSTEM,
      G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
      "org.bluez",
      "/",
      NULL,
      NULL,
      NULL,
      NULL,
      OnManagerCreatedThunk,
      this);
}

void BluetoothInstance::HandleGetDefaultAdapter(
    const picojson::value& msg) {
  if (adapter_info_.empty())
    return;

  picojson::value::object o;
  o["cmd"] = picojson::value("");
  o["reply_id"] = picojson::value(msg.get("reply_id").to_str());

  o["name"] = picojson::value(adapter_info_["Name"]);
  o["address"] = picojson::value(adapter_info_["Address"]);

  bool powered = (adapter_info_["Powered"] == "true");
  o["powered"] = picojson::value(powered);

  bool visible = (adapter_info_["Discoverable"] == "true");
  o["visible"] = picojson::value(visible);

  // This is the JS API entry point, so we should clean our message queue
  // on the next InternalPostMessage call.
  if (!is_js_context_initialized_)
    is_js_context_initialized_ = true;

  InternalSetSyncReply(picojson::value(o));
}

GDBusProxy* BluetoothInstance::CreateDeviceProxy(GAsyncResult* res) {
  GError* error = 0;
  GDBusProxy* deviceProxy = g_dbus_proxy_new_for_bus_finish(res, &error);
  if (deviceProxy) {
    known_devices_[g_dbus_proxy_get_object_path(deviceProxy)] = deviceProxy;
    g_signal_connect(deviceProxy, "g-properties-changed",
        G_CALLBACK(BluetoothInstance::OnPropertiesChanged), this);
  } else {
    g_printerr("## DeviceProxy creation error: %s\n", error->message);
    g_error_free(error);
  }

  return deviceProxy;
}

static void getPropertiesFromProxy(GDBusProxy* deviceProxy,
    picojson::value::object& o) {
  char** property_names = g_dbus_proxy_get_cached_property_names(deviceProxy);
  for (int i = 0; property_names && property_names[i]; i++) {
    GVariant* value = g_dbus_proxy_get_cached_property(deviceProxy,
        property_names[i]);

    getPropertyValue(property_names[i], value, o);

    g_variant_unref(value);
  }
  g_strfreev(property_names);
}

void BluetoothInstance::DeviceFound(GObject*, GAsyncResult* res) {
  GDBusProxy* deviceProxy = CreateDeviceProxy(res);

  if (deviceProxy) {
    picojson::value::object o;
    o["cmd"] = picojson::value("DeviceFound");

    getPropertiesFromProxy(deviceProxy, o);

    o["found_on_discovery"] = picojson::value(true);

    picojson::value v(o);
    InternalPostMessage(v);
  }
}

void BluetoothInstance::DeviceRemoved(GDBusObject* object) {
  if (!object)
    return;

  DeviceMap::iterator it =
      known_devices_.find(g_dbus_object_get_object_path(object));

  if (it == known_devices_.end())
    return;

  picojson::value::object o;
  o["cmd"] = picojson::value("DeviceRemoved");

  GVariant* value = g_dbus_proxy_get_cached_property(it->second, "Address");
  char* value_str = g_variant_print(value, true);
  o["Address"] = picojson::value(value_str);

  picojson::value v(o);
  InternalPostMessage(v);

  g_object_unref(it->second);
  known_devices_.erase(it);

  g_free(value_str);
  g_variant_unref(value);
}

void BluetoothInstance::KnownDeviceFound(GObject*, GAsyncResult* res) {
  GDBusProxy* deviceProxy = CreateDeviceProxy(res);

  if (deviceProxy) {
    picojson::value::object o;
    o["cmd"] = picojson::value("DeviceFound");

    getPropertiesFromProxy(deviceProxy, o);

    o["found_on_discovery"] = picojson::value(false);

    picojson::value v(o);
    InternalPostMessage(v);
  }
}

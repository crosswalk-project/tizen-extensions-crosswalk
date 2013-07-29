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
  } else {
    char* value_str = g_variant_print(value, TRUE);
    o[key_str] = picojson::value(value_str);
    g_free(value_str);
  }
}

void BluetoothContext::OnPropertiesChanged(GDBusProxy* proxy,
    GVariant* changed_properties, const gchar* const* invalidated_properties,
    gpointer user_data) {

  std::string interface(g_dbus_proxy_get_interface_name(proxy));
  BluetoothContext* handler = reinterpret_cast<BluetoothContext*>(user_data);

  if (g_variant_n_children(changed_properties) > 0) {
    GVariantIter* iter;
    const gchar* key;
    GVariant* value;
    picojson::value::object o;

    g_variant_get(changed_properties, "a{sv}", &iter);

    if (interface == "org.bluez.Device1") {
      std::map<std::string, GDBusProxy*>::iterator it =
          handler->known_devices_.find(g_dbus_proxy_get_object_path(proxy));

      if (it == handler->known_devices_.end())
        return;

      o["cmd"] = picojson::value("DeviceUpdated");

      while (g_variant_iter_loop(iter, "{&sv}", &key, &value))
        getPropertyValue(key, value, o);

      GVariant* addr = g_dbus_proxy_get_cached_property(it->second, "Address");
      char* str = g_variant_print(addr, TRUE);
      o["Address"] = picojson::value(str);
      g_free(str);
      g_variant_unref(addr);
    } else if (interface == "org.bluez.Adapter1") {
      o["cmd"] = picojson::value("AdapterUpdated");

      while (g_variant_iter_loop(iter, "{&sv}", &key, &value)) {
        getPropertyValue(key, value, o);
        handler->adapter_info_[key] = o[key].get<std::string>();
      }
    }
    g_variant_iter_free(iter);
    picojson::value v(o);
    handler->PostMessage(v);
  }
}

void BluetoothContext::OnDBusObjectAdded(GDBusObjectManager* manager,
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

void BluetoothContext::OnDBusObjectRemoved(GDBusObjectManager* manager,
    GDBusObject* object) {
  GDBusInterface* interface = g_dbus_object_get_interface(object,
      "org.bluez.Device1");

  if (interface) {
    DeviceRemoved(object);
    g_object_unref(interface);
  }
}

void BluetoothContext::OnDiscoveryStarted(GObject*, GAsyncResult* res) {
  GError* error = NULL;

  GVariant* result = g_dbus_proxy_call_finish(adapter_proxy_, res, &error);

  picojson::value::object o;
  o["cmd"] = picojson::value("");
  o["reply_id"] = picojson::value(discover_callback_id_);

  double errorCode = 0.0;
  if (result == NULL) {
    g_printerr ("Error discovering: %s\n", error->message);
    g_error_free(error);
    errorCode = 1.0; /* FIXME(jeez): error*/
  }

  o["error"] = picojson::value(errorCode);

  picojson::value v(o);
  PostMessage(v);

  if (result)
    g_variant_unref(result);
}

void BluetoothContext::OnDiscoveryStopped(GObject* source, GAsyncResult* res) {
  GError* error = NULL;
  GVariant* result = g_dbus_proxy_call_finish(adapter_proxy_, res, &error);

  int e = 0;
  if (result == NULL) {
    g_printerr ("Error stop discovery: %s\n", error->message);
    g_error_free(error);
    e = 1;
  }

  picojson::value::object o;
  o["cmd"] = picojson::value("");
  o["reply_id"] = picojson::value(stop_discovery_callback_id_);
  o["error"] = picojson::value(static_cast<double>(e));
  picojson::value v(o);
  PostMessage(v);
}

void BluetoothContext::OnAdapterProxyCreated(GObject*, GAsyncResult* res) {
  GError* error = NULL;
  adapter_proxy_ = g_dbus_proxy_new_for_bus_finish(res, &error);

  if (adapter_proxy_ == NULL) {
    g_printerr("## adapter_proxy_ creation error: %s\n", error->message);
    g_error_free(error);
    return;
  }

  gchar** properties = g_dbus_proxy_get_cached_property_names(adapter_proxy_);

  for (unsigned int n = 0; properties != NULL && properties[n] != NULL; n++) {
    const gchar* key = properties[n];
    GVariant* value = g_dbus_proxy_get_cached_property(adapter_proxy_, key);
    gchar* value_str = g_variant_print(value, TRUE);

    adapter_info_[key] = value_str;
    g_variant_unref(value);
    g_free(value_str);
  }

  g_signal_connect(adapter_proxy_, "g-properties-changed",
      G_CALLBACK(BluetoothContext::OnPropertiesChanged), this);

  g_strfreev(properties);
}

void BluetoothContext::CacheManagedObject(gpointer data, gpointer user_data)
{
  GDBusObject* object = static_cast<GDBusObject*>(data);
  GDBusInterface* interface = g_dbus_object_get_interface(object,
      "org.bluez.Device1");

  if (interface) {
    g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
        NULL /* GDBusInterfaceInfo */, "org.bluez",
        g_dbus_object_get_object_path(object), "org.bluez.Device1",
        NULL /* GCancellable */, BluetoothContext::KnownDeviceFoundThunk,
        user_data);

    g_object_unref(interface);
  }
}

void BluetoothContext::OnObjectManagerCreated(GObject*, GAsyncResult* res) {
  GError* err = NULL;
  object_manager_ = g_dbus_object_manager_client_new_for_bus_finish(res, &err);

  if (object_manager_ == NULL) {
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

BluetoothContext::~BluetoothContext() {
  delete api_;

  if (adapter_proxy_ != NULL)
    g_object_unref(adapter_proxy_);
  if (object_manager_ != NULL)
    g_object_unref(object_manager_);

  std::map<std::string, GDBusProxy*>::iterator it;
  for (it = known_devices_.begin(); it != known_devices_.end(); ++it)
    g_object_unref(it->second);
}

void BluetoothContext::PlatformInitialize() {
  adapter_proxy_ = NULL;
  object_manager_ = NULL;
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
      OnObjectManagerCreatedThunk,
      this);
}

void BluetoothContext::HandleDiscoverDevices(const picojson::value& msg) {
  discover_callback_id_ = msg.get("reply_id").to_str();
  if (adapter_proxy_ != NULL) {
    g_dbus_proxy_call(adapter_proxy_, "StartDiscovery", NULL,
        G_DBUS_CALL_FLAGS_NONE, 20000, NULL, OnDiscoveryStartedThunk, this);
  }
}

void BluetoothContext::HandleStopDiscovery(const picojson::value& msg) {
  stop_discovery_callback_id_ = msg.get("reply_id").to_str();
  if (adapter_proxy_ != NULL)
    g_dbus_proxy_call(adapter_proxy_, "StopDiscovery", NULL,
        G_DBUS_CALL_FLAGS_NONE, 20000, NULL, OnDiscoveryStoppedThunk, this);
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

GDBusProxy* BluetoothContext::CreateDeviceProxy(GAsyncResult* res) {
  GError* error = NULL;
  GDBusProxy* deviceProxy = g_dbus_proxy_new_for_bus_finish(res, &error);
  if (deviceProxy) {
    known_devices_[g_dbus_proxy_get_object_path(deviceProxy)] = deviceProxy;
    g_signal_connect(deviceProxy, "g-properties-changed",
        G_CALLBACK(BluetoothContext::OnPropertiesChanged), this);
  } else {
    g_printerr("## DeviceProxy creation error: %s\n", error->message);
    g_error_free(error);
  }

  return deviceProxy;
}

static void getPropertiesFromProxy(GDBusProxy* deviceProxy,
    picojson::value::object& o)
{
  char** property_names = g_dbus_proxy_get_cached_property_names(deviceProxy);
  for (int i = 0; property_names != NULL && property_names[i] != NULL; i++) {
    GVariant* value = g_dbus_proxy_get_cached_property(deviceProxy,
        property_names[i]);

    getPropertyValue(property_names[i], value, o);

    g_variant_unref(value);
  }
  g_strfreev(property_names);
}

void BluetoothContext::DeviceFound(GObject*, GAsyncResult* res) {
  GDBusProxy* deviceProxy = CreateDeviceProxy(res);

  if (deviceProxy) {
    picojson::value::object o;
    o["cmd"] = picojson::value("DeviceFound");

    getPropertiesFromProxy(deviceProxy, o);

    o["found_on_discovery"] = picojson::value(true);

    picojson::value v(o);
    PostMessage(v);
  }
}

void BluetoothContext::DeviceRemoved(GDBusObject* object) {
  if (object) {
    std::map<std::string, GDBusProxy*>::iterator it =
        known_devices_.find(g_dbus_object_get_object_path(object));

    if (it == known_devices_.end())
      return;

    picojson::value::object o;
    o["cmd"] = picojson::value("DeviceRemoved");

    GVariant* value = g_dbus_proxy_get_cached_property(it->second, "Address");
    char* value_str = g_variant_print(value, TRUE);
    o["Address"] = picojson::value(value_str);

    picojson::value v(o);
    PostMessage(v);

    g_object_unref(it->second);
    known_devices_.erase(it);

    g_free(value_str);
    g_variant_unref(value);
  }
}

void BluetoothContext::KnownDeviceFound(GObject*, GAsyncResult* res) {
  GDBusProxy* deviceProxy = CreateDeviceProxy(res);

  if (deviceProxy) {
    picojson::value::object o;
    o["cmd"] = picojson::value("DeviceFound");

    getPropertiesFromProxy(deviceProxy, o);

    o["found_on_discovery"] = picojson::value(false);

    picojson::value v(o);
    PostMessage(v);
  }
}

void BluetoothContext::FlushPendingMessages() {
  // Flush previous pending messages.
  if (!queue_.empty()) {
    MessageQueue::iterator it;
    for (it = queue_.begin(); it != queue_.end(); ++it)
      api_->PostMessage((*it).serialize().c_str());
  }
}

void BluetoothContext::PostMessage(picojson::value v) {
  // If the JavaScript 'context' hasn't been initialized yet (i.e. the C++
  // backend was loaded and it is already executing but
  // tizen.bluetooth.getDefaultAdapter() hasn't been called so far), we need to
  // queue the PostMessage calls and defer them until the default adapter is set
  // on the JS side. That will guarantee the correct callbacks will be called,
  // and on the right order, only after tizen.bluetooth.getDefaultAdapter() is
  // called.
  if (!is_js_context_initialized_) {
    queue_.push_back(v);
    return;
  }

  FlushPendingMessages();
  api_->PostMessage(v.serialize().c_str());
}

void BluetoothContext::SetSyncReply(picojson::value v) {
  std::string cmd = v.get("cmd").to_str();
  if (cmd == "GetDefaultAdapter")
    api_->SetSyncReply(HandleGetDefaultAdapter(v).serialize().c_str());

  FlushPendingMessages();
}

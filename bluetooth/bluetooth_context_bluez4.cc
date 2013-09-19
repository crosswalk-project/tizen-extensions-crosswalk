// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bluetooth/bluetooth_context.h"
#include "common/picojson.h"

#if defined(TIZEN_MOBILE)
#include <bluetooth.h>
#endif

#include <list>

#include <sys/types.h>
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

namespace {

static std::list<GCancellable*> cancellables;

static GCancellable* new_cancellable() {
  GCancellable* cancellable = g_cancellable_new();

  cancellables.push_back(cancellable);;

  return cancellable;
}

} // namespace

#define RFCOMM_RECORD "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>	\
<record>								\
  <attribute id=\"0x0001\">						\
    <sequence>								\
      <uuid value=\"%s\"/>						\
    </sequence>								\
  </attribute>								\
									\
  <attribute id=\"0x0004\">						\
    <sequence>								\
      <sequence>							\
        <uuid value=\"0x0100\"/>					\
      </sequence>							\
      <sequence>							\
        <uuid value=\"0x0003\"/>					\
        <uint8 value=\"%u\" name=\"channel\"/>				\
      </sequence>							\
    </sequence>								\
  </attribute>								\
									\
  <attribute id=\"0x0100\">						\
    <text value=\"%s\" name=\"name\"/>					\
  </attribute>								\
</record>"

static uint32_t rfcomm_get_channel(int fd) {
  struct sockaddr_rc laddr;
  socklen_t alen = sizeof(laddr);

  memset(&laddr, 0, alen);

  if (getsockname(fd, (struct sockaddr *) &laddr, &alen) < 0)
    return 0;

  return laddr.rc_channel;
}

static int rfcomm_get_peer(int fd, char* address) {
  if (!address)
    return -1;

  struct sockaddr_rc raddr;
  socklen_t alen = sizeof(raddr);

  memset(&raddr, 0, alen);

  if (getpeername(fd, (struct sockaddr *) &raddr, &alen) < 0)
    return -1;

  ba2str(&raddr.rc_bdaddr, address);

  return 0;
}

// Returns an fd listening on 'channel' RFCOMM Channel
static int rfcomm_listen(uint8_t *channel) {
  int sk = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
  if (sk < 0)
    return -1;

  struct sockaddr_rc laddr;
  // All zeros means BDADDR_ANY and any channel.
  memset(&laddr, 0, sizeof(laddr));
  laddr.rc_family = AF_BLUETOOTH;

  if (bind(sk, (struct sockaddr *) &laddr, sizeof(laddr)) < 0) {
    close(sk);
    return -1;
  }

  listen(sk, 10);

  if (channel)
    *channel = rfcomm_get_channel(sk);

  return sk;
}

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
                                 handler->all_pending_,
                                 OnDeviceProxyCreatedThunk,
                                 CancellableWrap(handler->all_pending_, data));
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
                                 all_pending_,
                                 OnDeviceProxyCreatedThunk,
                                 CancellableWrap(all_pending_, this));
      }

      g_variant_iter_free(iter);
    } else {
      if (g_variant_is_of_type(value, G_VARIANT_TYPE_STRING))
        adapter_info_[key] = std::string(g_variant_get_string(value, NULL));
      else
        adapter_info_[key] = g_variant_print(value, false);
    }
  }

  // We didn't have the information when getDefaultAdapter was called,
  // replying now.
  if (!default_adapter_reply_id_.empty()) {
    picojson::value::object o;

    o["reply_id"] = picojson::value(default_adapter_reply_id_);
    default_adapter_reply_id_.clear();

    is_js_context_initialized_ = true;

    AdapterInfoToValue(o);
    SetSyncReply(picojson::value(o));
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
                    G_DBUS_CALL_FLAGS_NONE, 5000, all_pending_, OnGotAdapterPropertiesThunk,
                    CancellableWrap(all_pending_, this));

  g_signal_connect(adapter_proxy_, "g-signal",
    G_CALLBACK(BluetoothContext::OnSignal), this);
}

void BluetoothContext::OnServiceProxyCreated(GObject*, GAsyncResult* res) {
  GError* error = 0;
  service_proxy_ = g_dbus_proxy_new_for_bus_finish(res, &error);

  if (!service_proxy_) {
    g_printerr("\n\n## adapter_proxy_ creation error: %s\n", error->message);
    g_error_free(error);
  }
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
                    G_DBUS_CALL_FLAGS_NONE, 5000, all_pending_, OnGotDefaultAdapterPathThunk,
                    CancellableWrap(all_pending_, this));
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
      all_pending_, /* GCancellable */
      OnAdapterProxyCreatedThunk,
      CancellableWrap(all_pending_, this));

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
      G_DBUS_PROXY_FLAGS_NONE,
      NULL, /* GDBusInterfaceInfo */
      "org.bluez",
      path,
      "org.bluez.Service",
      all_pending_, /* GCancellable */
      OnServiceProxyCreatedThunk,
      CancellableWrap(all_pending_, this));

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
                    G_DBUS_CALL_FLAGS_NONE, -1, all_pending_, OnAdapterDestroyBondingThunk,
                    CancellableWrap(all_pending_, this));

  g_variant_unref(result);
}

BluetoothContext::~BluetoothContext() {
  delete api_;

  g_cancellable_cancel(all_pending_);
  // Explicitly leaking all_pending_ here. It will be free'd on 'shutdown'.

  if (adapter_proxy_)
    g_object_unref(adapter_proxy_);

  if (manager_proxy_)
    g_object_unref(manager_proxy_);

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

  pending_listen_socket_ = -1;

  rfcomm_listener_ = g_socket_listener_new();

  is_js_context_initialized_ = false;

  all_pending_ = new_cancellable();

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
      G_DBUS_PROXY_FLAGS_NONE,
      NULL, /* GDBusInterfaceInfo */
      "org.bluez",
      "/",
      "org.bluez.Manager",
      all_pending_, /* GCancellable */
      OnManagerCreatedThunk,
      CancellableWrap(all_pending_, this));
}

void BluetoothContext::HandleGetDefaultAdapter(const picojson::value& msg) {
  // We still don't have the information. It was requested during
  // initialization, so it should arrive eventually.
  if (adapter_info_.empty()) {
    default_adapter_reply_id_ = msg.get("reply_id").to_str();
    return;
  }

  picojson::value::object o;

  o["reply_id"] = picojson::value(msg.get("reply_id").to_str());
  AdapterInfoToValue(o);

  // This is the JS API entry point, so we should clean our message queue
  // on the next PostMessage call.
  if (!is_js_context_initialized_)
    is_js_context_initialized_ = true;

  SetSyncReply(picojson::value(o));
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
                        g_variant_new("(sv)", "DiscoverableTimeout",
                                      g_variant_new("u", timeout)),
                        G_DBUS_CALL_FLAGS_NONE, 5000, all_pending_, NULL,
                        CancellableWrap(all_pending_, NULL));
    }
  } else if (property == "Powered")
    value = g_variant_new("b", msg.get("value").get<bool>());

  assert(value);

  callbacks_map_[property] = msg.get("reply_id").to_str();

  OnAdapterPropertySetData* property_set_callback_data_ =
      new OnAdapterPropertySetData;
  property_set_callback_data_->property = property;
  property_set_callback_data_->bt_context = this;
  property_set_callback_data_->cancellable = all_pending_;

  g_dbus_proxy_call(adapter_proxy_, "SetProperty",
                    g_variant_new("(sv)", property.c_str(), value),
                    G_DBUS_CALL_FLAGS_NONE, 5000, all_pending_, OnAdapterPropertySetThunk,
                    property_set_callback_data_);
}

void BluetoothContext::HandleCreateBonding(const picojson::value& msg) {
  std::string address = msg.get("address").to_str();
  callbacks_map_["CreateBonding"] = msg.get("reply_id").to_str();

  g_dbus_proxy_call(adapter_proxy_, "CreatePairedDevice",
                    g_variant_new ("(sos)", address.c_str(), "/", "KeyboardDisplay"),
                    G_DBUS_CALL_FLAGS_NONE, -1, all_pending_, OnAdapterCreateBondingThunk,
                    CancellableWrap(all_pending_, this));
}

void BluetoothContext::HandleDestroyBonding(const picojson::value& msg) {
  std::string address = msg.get("address").to_str();
  callbacks_map_["DestroyBonding"] = msg.get("reply_id").to_str();

  g_dbus_proxy_call(adapter_proxy_, "FindDevice",
                    g_variant_new("(s)", address.c_str()),
                    G_DBUS_CALL_FLAGS_NONE, -1, all_pending_, OnFoundDeviceThunk,
                    CancellableWrap(all_pending_, this));
}

gboolean BluetoothContext::OnSocketHasData(GSocket* client, GIOCondition cond,
                                              gpointer user_data) {

  if (cond & G_IO_ERR || cond & G_IO_HUP) {
    // FIXME(vcgomes): Notify that the socket has closed
    return false;
  }

  BluetoothContext* handler = reinterpret_cast<BluetoothContext*>(user_data);

  int fd = g_socket_get_fd(client);
  gchar buf[512];
  gssize len;

  len = g_socket_receive(client, buf, sizeof(buf), NULL, NULL);
  if (len < 0)
    return false;

  picojson::value::object o;

  o["cmd"] = picojson::value("SocketHasData");
  o["socket_fd"] = picojson::value(static_cast<double>(fd));
  o["data"] = picojson::value(buf, len);

  handler->PostMessage(picojson::value(o));

  return true;
}

void BluetoothContext::OnListenerAccept(GObject* object, GAsyncResult* res) {
  GError* error = 0;
  GSocket *socket = g_socket_listener_accept_socket_finish(rfcomm_listener_, res,
                                                           NULL, &error);
  if (!socket) {
    g_printerr("\n\nlistener_accept_socket_finish failed %s\n", error->message);
    return;
  }

  int fd = g_socket_get_fd(socket);
  uint32_t channel = rfcomm_get_channel(fd);
  char address[18]; // "XX:XX:XX:XX:XX:XX"
  picojson::value::object o;

  rfcomm_get_peer(fd, address);

  o["cmd"] = picojson::value("RFCOMMSocketAccept");
  o["channel"] = picojson::value(static_cast<double>(channel));
  o["socket_fd"] = picojson::value(static_cast<double>(fd));
  o["peer"] = picojson::value(address);

  PostMessage(picojson::value(o));

  GSource *source = g_socket_create_source(socket, G_IO_IN, NULL);

  g_source_set_callback(source, (GSourceFunc) BluetoothContext::OnSocketHasData,
                        this, NULL);
  g_source_attach(source, NULL);
  g_source_unref(source);
}

void BluetoothContext::OnServiceAddRecord(GObject* object, GAsyncResult* res) {
  GError* error = 0;
  picojson::value::object o;
  GVariant* result = g_dbus_proxy_call_finish(service_proxy_, res, &error);

  o["cmd"] = picojson::value("");
  o["reply_id"] = picojson::value(callbacks_map_["RFCOMMListen"]);

  if (!result) {
    o["error"] = picojson::value(static_cast<double>(1));

    close(pending_listen_socket_);

    pending_listen_socket_ = -1;

    g_printerr("\n\nError OnServiceAddRecord: %s\n", error->message);
    g_error_free(error);
  } else {
    uint32_t handle;
    int sk = pending_listen_socket_;
    pending_listen_socket_ = -1;

    GSocket *socket = g_socket_new_from_fd(sk, NULL);
    g_socket_set_blocking(socket, false);

    g_socket_listener_add_socket(rfcomm_listener_, socket, NULL, NULL);

    g_socket_listener_accept_async(rfcomm_listener_, NULL,
                                   OnListenerAcceptThunk, this);

    g_variant_get(result, "(u)", &handle);

    o["error"] = picojson::value(static_cast<double>(0));
    o["server_fd"] = picojson::value(static_cast<double>(sk));
    o["sdp_handle"] = picojson::value(static_cast<double>(handle));
    o["channel"] = picojson::value(static_cast<double>(rfcomm_get_channel(sk)));
  }

  callbacks_map_.erase("RFCOMMListen");
  g_variant_unref(result);

  PostMessage(picojson::value(o));
}

void BluetoothContext::HandleRFCOMMListen(const picojson::value& msg) {
  std::string name = msg.get("name").to_str();
  std::string uuid = msg.get("uuid").to_str();
  uint8_t channel = 0;
  int sk;

  // FIXME(vcgomes): Error handling
  if (pending_listen_socket_ >= 0)
    return;

  sk = rfcomm_listen(&channel);
  if (sk < 0)
    return;

  callbacks_map_["RFCOMMListen"] = msg.get("reply_id").to_str();

  pending_listen_socket_ = sk;

  char *record = g_strdup_printf(RFCOMM_RECORD, uuid.c_str(), channel, name.c_str());

  g_dbus_proxy_call(service_proxy_, "AddRecord", g_variant_new("(s)", record),
                    G_DBUS_CALL_FLAGS_NONE, -1, all_pending_,
                    OnServiceAddRecordThunk, CancellableWrap(all_pending_, this));
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
                    G_DBUS_CALL_FLAGS_NONE, 5000, all_pending_, OnGotDevicePropertiesThunk,
                    CancellableWrap(all_pending_, this));

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

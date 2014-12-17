// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bluetooth/bluetooth_instance.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include <list>

#include "common/picojson.h"
#include "tizen/tizen.h"

// FIXME: C++0x removed support for typeof. bluetooth.h requires it, so until
// bluetooth.h is fixed to use something future safe, use the GCC intrinsic
// __typeof__ as replacement.
#define typeof(x) __typeof__(x)

namespace {

static std::list<GCancellable*> cancellables;

static GCancellable* new_cancellable() {
  GCancellable* cancellable = g_cancellable_new();

  cancellables.push_back(cancellable);

  return cancellable;
}

}  // namespace

const char* kRFCOMMRecord =
    "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
    "<record>\n"
    "  <attribute id=\"0x0001\">\n"
    "    <sequence>\n"
    "      <uuid value=\"%s\"/>\n"
    "    </sequence>\n"
    "  </attribute>\n"
    "\n"
    "  <attribute id=\"0x0004\">\n"
    "    <sequence>\n"
    "      <sequence>\n"
    "        <uuid value=\"0x0100\"/>\n"
    "      </sequence>\n"
    "      <sequence>\n"
    "        <uuid value=\"0x0003\"/>\n"
    "        <uint8 value=\"%u\" name=\"channel\"/>\n"
    "      </sequence>\n"
    "    </sequence>\n"
    "  </attribute>\n"
    "\n"
    "  <attribute id=\"0x0100\">\n"
    "    <text value=\"%s\" name=\"name\"/>\n"
    "  </attribute>\n"
    "</record>";

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
    o["ClassMajor"] = picojson::value \
                      (static_cast<double>((class_id & 0x00001F00) >> 8));
    o["ClassMinor"] = picojson::value \
                      (static_cast<double>(class_id & 0x000000FC));
    o["ClassService"] = picojson::value\
                        (static_cast<double>(class_id & 0x00FF0000));
  } else if (!strcmp(key, "RSSI")) {
    gint16 class_id = g_variant_get_int16(value);
    o[key] = picojson::value(static_cast<double>(class_id));
  } else if (strcmp(key, "Devices")) {
    // FIXME(jeez): Handle 'Devices' property.
    std::string value_str;
    if (g_variant_is_of_type(value, G_VARIANT_TYPE_STRING))
      value_str = g_variant_get_string(value, NULL);
    else
      value_str = g_variant_print(value, false);
    o[key] = picojson::value(value_str);
  }
}

void BluetoothInstance::OnSignal(GDBusProxy* proxy, gchar* sender,
    gchar* signal, GVariant* parameters, gpointer data) {
  BluetoothInstance* handler = reinterpret_cast<BluetoothInstance*>(data);

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
      handler->adapter_info_[name] = g_variant_print(value, false);

      picojson::value::object property_updated;
      property_updated["cmd"] = picojson::value("AdapterUpdated");
      property_updated[name] = picojson::value(handler->adapter_info_[name]);
      handler->InternalPostMessage(picojson::value(property_updated));
    }
    g_variant_unref(value);
  }
}

void BluetoothInstance::OnDeviceSignal(
    GDBusProxy* proxy, gchar* sender, gchar* signal,
    GVariant* parameters, gpointer data) {
  // We only want PropertyChanged ones.
  if (strcmp(signal, "PropertyChanged"))
    return;

  const char* iface = g_dbus_proxy_get_interface_name(proxy);

  // We only want org.bluez.Device signals.
  if (strcmp(iface, "org.bluez.Device"))
    return;

  const char* path = g_dbus_proxy_get_object_path(proxy);
  BluetoothInstance* handler = reinterpret_cast<BluetoothInstance*>(data);

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

  handler->InternalPostMessage(picojson::value(o));
}

// static
void BluetoothInstance::OnManagerSignal(GDBusProxy* proxy, gchar* sender_name,
                                        gchar* signal, GVariant* parameters,
                                        gpointer user_data) {
  // We only want DefaultAdapterChanged ones
  if (strcmp(signal, "DefaultAdapterChanged"))
    return;

  const char* iface = g_dbus_proxy_get_interface_name(proxy);

  // We only want org.bluez.Manager signals.
  if (strcmp(iface, "org.bluez.Manager"))
    return;

  BluetoothInstance* handler = reinterpret_cast<BluetoothInstance*>(user_data);
  const char* path;
  g_variant_get(parameters, "(o)", &path);

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
                           G_DBUS_PROXY_FLAGS_NONE,
                           NULL, /* GDBusInterfaceInfo */
                           "org.bluez",
                           path,
                           "org.bluez.Adapter",
                           handler->all_pending_, /* GCancellable */
                           OnAdapterProxyCreatedThunk,
                           CancellableWrap(handler->all_pending_, handler));

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
                           G_DBUS_PROXY_FLAGS_NONE,
                           NULL, /* GDBusInterfaceInfo */
                           "org.bluez",
                           path,
                           "org.bluez.Service",
                           handler->all_pending_, /* GCancellable */
                           OnServiceProxyCreatedThunk,
                           CancellableWrap(handler->all_pending_, handler));
}

void BluetoothInstance::OnGotAdapterProperties(GObject*, GAsyncResult* res) {
  GError* error = 0;
  GVariant* result = g_dbus_proxy_call_finish(adapter_proxy_, res, &error);

  if (!result) {
    AdapterSendGetDefaultAdapterReply();
    g_printerr("\n\nError Got DefaultAdapter Properties: %s\n", error->message);
    g_error_free(error);
    return;
  }

  const gchar* key;
  GVariant* value;
  GVariantIter* it;
  g_variant_get(result, "(a{sv})", &it);

  picojson::value::object o;
  o["cmd"] = picojson::value("AdapterUpdated");

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

      o[key] = picojson::value(adapter_info_[key]);
    }
  }

  InternalPostMessage(picojson::value(o));

  AdapterSendGetDefaultAdapterReply();

  auto map_it = callbacks_map_.find("Powered");
  if (map_it != callbacks_map_.end()) {
    o.clear();

    o["cmd"] = picojson::value("");
    o["reply_id"] = picojson::value(callbacks_map_["Powered"]);
    o["error"] = picojson::value(static_cast<double>(NO_ERROR));

    InternalPostMessage(picojson::value(o));

    callbacks_map_.erase("Powered");
  }

  g_variant_iter_free(it);
}

void BluetoothInstance::OnAdapterPropertySet(
    std::string property, GAsyncResult* res) {
  GError* error = 0;
  auto it = callbacks_map_.find(property);

  if (it == callbacks_map_.end())
    return;

  GVariant* result = g_dbus_proxy_call_finish(adapter_proxy_, res, &error);

  picojson::value::object o;
  o["cmd"] = picojson::value("");
  o["reply_id"] = picojson::value(callbacks_map_[property]);

  if (!result) {
    g_printerr("\n\nError Got DefaultAdapter Property SET: %s\n",
               error->message);
    g_error_free(error);
    // No matter the error info here, BlueZ4's documentation says the only
    // error that can be raised here is org.bluez.Error.InvalidArguments.
    o["error"] = picojson::value(static_cast<double>(INVALID_VALUES_ERR));
  } else {
    o["error"] = picojson::value(static_cast<double>(NO_ERROR));
  }

  InternalPostMessage(picojson::value(o));
  callbacks_map_.erase(property);

  g_variant_unref(result);
}

void BluetoothInstance::OnAdapterProxyCreated(GObject*, GAsyncResult* res) {
  GError* error = 0;
  adapter_proxy_ = g_dbus_proxy_new_for_bus_finish(res, &error);

  if (!adapter_proxy_) {
    AdapterSendGetDefaultAdapterReply();
    g_printerr("\n\n## adapter_proxy_ creation error: %s\n", error->message);
    g_error_free(error);
    return;
  }

  g_dbus_proxy_call(adapter_proxy_, "GetProperties", NULL,
                    G_DBUS_CALL_FLAGS_NONE, 5000, all_pending_,
                    OnGotAdapterPropertiesThunk,
                    CancellableWrap(all_pending_, this));

  g_signal_connect(adapter_proxy_, "g-signal",
    G_CALLBACK(BluetoothInstance::OnSignal), this);
}

void BluetoothInstance::OnServiceProxyCreated(GObject*, GAsyncResult* res) {
  GError* error = 0;
  service_proxy_ = g_dbus_proxy_new_for_bus_finish(res, &error);

  if (!service_proxy_) {
    g_printerr("\n\n## adapter_proxy_ creation error: %s\n", error->message);
    g_error_free(error);
  }
}

void BluetoothInstance::OnManagerCreated(GObject*, GAsyncResult* res) {
  GError* err = 0;
  manager_proxy_ = g_dbus_proxy_new_for_bus_finish(res, &err);

  if (!manager_proxy_) {
    AdapterSendGetDefaultAdapterReply();
    g_printerr("## Manager Proxy creation error: %s\n", err->message);
    g_error_free(err);
    return;
  }

  g_dbus_proxy_call(manager_proxy_, "DefaultAdapter", NULL,
                    G_DBUS_CALL_FLAGS_NONE, 5000, all_pending_,
                    OnGotDefaultAdapterPathThunk,
                    CancellableWrap(all_pending_, this));

  g_signal_connect(manager_proxy_, "g-signal",
                   G_CALLBACK(BluetoothInstance::OnManagerSignal), this);
}

// static
void BluetoothInstance::OnBluetoothServiceAppeared(GDBusConnection* connection,
                                                   const char* name,
                                                   const char* name_owner,
                                                   void* user_data) {
  BluetoothInstance* handler = reinterpret_cast<BluetoothInstance*>(user_data);

  if (handler->manager_proxy_)
    return;

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
                           G_DBUS_PROXY_FLAGS_NONE,
                           NULL, /* GDBusInterfaceInfo */
                           "org.bluez",
                           "/",
                           "org.bluez.Manager",
                           handler->all_pending_, /* GCancellable */
                           OnManagerCreatedThunk,
                           CancellableWrap(handler->all_pending_, handler));
}

// static
void BluetoothInstance::OnBluetoothServiceVanished(GDBusConnection* connection,
                                                   const char* name,
                                                   void* user_data) {
  BluetoothInstance* handler = reinterpret_cast<BluetoothInstance*>(user_data);

  if (handler->manager_proxy_) {
    g_object_unref(handler->manager_proxy_);
    handler->manager_proxy_ = 0;
  }

  if (handler->adapter_proxy_) {
    g_object_unref(handler->adapter_proxy_);
    handler->adapter_proxy_ = 0;
  }

  handler->AdapterSendGetDefaultAdapterReply();
}

void BluetoothInstance::AdapterSetPowered(const picojson::value& msg) {
  bool powered = msg.get("value").get<bool>();
  int error = NO_ERROR;

  OnAdapterPropertySetData* property_set_callback_data_ =
      new OnAdapterPropertySetData;
  property_set_callback_data_->property = std::string("Powered");
  property_set_callback_data_->bt_context = this;
  property_set_callback_data_->cancellable = all_pending_;

  g_dbus_proxy_call(adapter_proxy_, "SetProperty",
                    g_variant_new("(sv)", "Powered",
                                  g_variant_new("b", powered)),
                    G_DBUS_CALL_FLAGS_NONE, 5000, all_pending_,
                    OnAdapterPropertySetThunk,
                    property_set_callback_data_);

  // Reply right away in case of error, or powered off.
  if (error || powered == false) {
    picojson::value::object o;

    o["cmd"] = picojson::value("");
    o["reply_id"] = msg.get("reply_id");
    o["error"] = picojson::value(static_cast<double>(error));

    InternalPostMessage(picojson::value(o));
    return;
  }

  callbacks_map_["Powered"] = msg.get("reply_id").to_str();
}

void BluetoothInstance::OnGotDefaultAdapterPath(GObject*, GAsyncResult* res) {
  GError* error = 0;
  GVariant* result = g_dbus_proxy_call_finish(manager_proxy_, res, &error);

  if (!result) {
    AdapterSendGetDefaultAdapterReply();
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

void BluetoothInstance::OnAdapterCreateBonding(GObject*, GAsyncResult* res) {
  GError* error = 0;
  GVariant* result = g_dbus_proxy_call_finish(adapter_proxy_, res, &error);

  picojson::value::object o;
  o["cmd"] = picojson::value("");
  o["reply_id"] = picojson::value(callbacks_map_["CreateBonding"]);
  o["error"] = picojson::value(static_cast<double>(NO_ERROR));

  if (!result) {
    g_printerr("\n\nError on creating adapter bonding: %s\n", error->message);
    g_error_free(error);

    o["error"] = picojson::value(static_cast<double>(UNKNOWN_ERR));
  } else {
    g_variant_unref(result);
  }

  InternalPostMessage(picojson::value(o));
  callbacks_map_.erase("CreateBonding");
}

void BluetoothInstance::OnAdapterDestroyBonding(GObject*, GAsyncResult* res) {
  GError* error = 0;
  GVariant* result = g_dbus_proxy_call_finish(adapter_proxy_, res, &error);

  picojson::value::object o;
  o["cmd"] = picojson::value("");
  o["reply_id"] = picojson::value(callbacks_map_["DestroyBonding"]);
  o["error"] = picojson::value(static_cast<double>(NO_ERROR));

  if (!result) {
    g_printerr("\n\nError on destroying adapter bonding: %s\n", error->message);
    g_error_free(error);

    o["error"] = picojson::value(static_cast<double>(UNKNOWN_ERR));
  } else {
    g_variant_unref(result);
  }

  InternalPostMessage(picojson::value(o));
  callbacks_map_.erase("DestroyBonding");
}

void BluetoothInstance::OnFoundDevice(GObject*, GAsyncResult* res) {
  picojson::value::object o;
  char* object_path;
  GError* error = 0;
  GVariant* result = g_dbus_proxy_call_finish(adapter_proxy_, res, &error);

  if (!result) {
    g_printerr("\n\nError on destroying adapter bonding: %s\n", error->message);
    g_error_free(error);

    o["cmd"] = picojson::value("");
    o["reply_id"] = picojson::value(callbacks_map_["DestroyBonding"]);
    o["error"] = picojson::value(static_cast<double>(UNKNOWN_ERR));

    InternalPostMessage(picojson::value(o));
    callbacks_map_.erase("DestroyBonding");
    return;
  }

  g_variant_get(result, "(o)", &object_path);
  g_dbus_proxy_call(adapter_proxy_, "RemoveDevice",
                    g_variant_new("(o)", object_path),
                    G_DBUS_CALL_FLAGS_NONE, -1, all_pending_,
                    OnAdapterDestroyBondingThunk,
                    CancellableWrap(all_pending_, this));

  g_variant_unref(result);
}

BluetoothInstance::~BluetoothInstance() {
  g_cancellable_cancel(all_pending_);
  // Explicitly leaking all_pending_ here. It will be free'd on 'shutdown'.

  if (adapter_proxy_)
    g_object_unref(adapter_proxy_);

  if (manager_proxy_)
    g_object_unref(manager_proxy_);

  DeviceMap::iterator it;
  for (it = known_devices_.begin(); it != known_devices_.end(); ++it)
    g_object_unref(it->second);

  g_bus_unwatch_name(name_watch_id_);
}

void BluetoothInstance::PlatformInitialize() {
  adapter_proxy_ = 0;
  manager_proxy_ = 0;

  pending_listen_socket_ = -1;

  rfcomm_listener_ = g_socket_listener_new();

  is_js_context_initialized_ = false;

  all_pending_ = new_cancellable();

  name_watch_id_ = g_bus_watch_name(G_BUS_TYPE_SYSTEM, "org.bluez",
                                    G_BUS_NAME_WATCHER_FLAGS_NONE,
                                    OnBluetoothServiceAppeared,
                                    OnBluetoothServiceVanished,
                                    this, NULL);

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM,
                           G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                           NULL, /* GDBusInterfaceInfo */
                           "org.bluez",
                           "/",
                           "org.bluez.Manager",
                           all_pending_, /* GCancellable */
                           OnManagerCreatedThunk,
                           CancellableWrap(all_pending_, this));
}

void BluetoothInstance::HandleGetDefaultAdapter(const picojson::value& msg) {
  default_adapter_reply_id_ = msg.get("reply_id").to_str();

  // We still don't have the information. It was requested during
  // initialization, so it should arrive eventually.
  if (adapter_info_.empty()) {
    return;
  }

  AdapterSendGetDefaultAdapterReply();
}

void BluetoothInstance::DeviceFound(std::string address,
                                    GVariantIter* properties) {
  const gchar* key;
  GVariant* value;
  picojson::value::object o;

  o["cmd"] = picojson::value("DeviceFound");
  o["found_on_discovery"] = picojson::value(true);

  while (g_variant_iter_loop(properties, "{sv}", &key, &value))
    getPropertyValue(key, value, o);

  picojson::value v(o);
  InternalPostMessage(v);
}

void BluetoothInstance::HandleSetAdapterProperty(const picojson::value& msg) {
  std::string property = msg.get("property").to_str();
  // We handle the Powered property differently because we may have to do
  // different things depending on the platform on which we are running.
  if (property == "Powered") {
    AdapterSetPowered(msg);
    return;
  }

  GVariant* value;
  if (property == "Name") {
    GVariant* name = g_variant_new("s", msg.get("value").to_str().c_str());
    value = g_variant_new("(sv)", property.c_str(), name);
    goto done;
  } else if (property == "Discoverable") {
    if (msg.contains("timeout")) {
      const guint32 timeout =
          static_cast<guint32>(msg.get("timeout").get<double>());
      GVariant* disc_timeout = g_variant_new("u", timeout);
      value = g_variant_new("(sv)", "DiscoverableTimeout", disc_timeout);
      goto done;
    }

    GVariant* discoverable = g_variant_new("b", msg.get("value").get<bool>());
    value = g_variant_new("(sv)", property.c_str(), discoverable);
    goto done;
  } else {
    // Unhandled property.
    return;
  }

 done:
  OnAdapterPropertySetData* property_set_callback_data_ =
      new OnAdapterPropertySetData;
  property_set_callback_data_->property = property;
  property_set_callback_data_->bt_context = this;
  property_set_callback_data_->cancellable = all_pending_;

  callbacks_map_[property] = msg.get("reply_id").to_str();

  g_dbus_proxy_call(adapter_proxy_, "SetProperty", value,
                    G_DBUS_CALL_FLAGS_NONE, 5000, all_pending_,
                    OnAdapterPropertySetThunk,
                    property_set_callback_data_);
}

void BluetoothInstance::HandleCreateBonding(const picojson::value& msg) {
  std::string address = msg.get("address").to_str();
  callbacks_map_["CreateBonding"] = msg.get("reply_id").to_str();

  g_dbus_proxy_call(
      adapter_proxy_, "CreatePairedDevice",
      g_variant_new("(sos)", address.c_str(), "/", "KeyboardDisplay"),
      G_DBUS_CALL_FLAGS_NONE, -1, all_pending_, OnAdapterCreateBondingThunk,
      CancellableWrap(all_pending_, this));
}

void BluetoothInstance::HandleDestroyBonding(const picojson::value& msg) {
  std::string address = msg.get("address").to_str();
  callbacks_map_["DestroyBonding"] = msg.get("reply_id").to_str();

  g_dbus_proxy_call(
      adapter_proxy_, "FindDevice",
      g_variant_new("(s)", address.c_str()),
      G_DBUS_CALL_FLAGS_NONE, -1, all_pending_, OnFoundDeviceThunk,
      CancellableWrap(all_pending_, this));
}

gboolean BluetoothInstance::OnSocketHasData(GSocket* client, GIOCondition cond,
                                            gpointer user_data) {
  BluetoothInstance* handler = reinterpret_cast<BluetoothInstance*>(user_data);
  int fd = g_socket_get_fd(client);
  picojson::value::object o;

  if (cond & G_IO_ERR || cond & G_IO_HUP) {
    o["cmd"] = picojson::value("SocketClosed");
    o["socket_fd"] = picojson::value(static_cast<double>(fd));

    handler->InternalPostMessage(picojson::value(o));

    return false;
  }

  gchar buf[512];
  gssize len;

  len = g_socket_receive(client, buf, sizeof(buf), NULL, NULL);
  if (len < 0)
    return false;

  o["cmd"] = picojson::value("SocketHasData");
  o["socket_fd"] = picojson::value(static_cast<double>(fd));
  o["data"] = picojson::value(buf, len);

  handler->InternalPostMessage(picojson::value(o));

  return true;
}

void BluetoothInstance::OnListenerAccept(GObject* object, GAsyncResult* res) {
  GError* error = 0;
  GSocket *socket = g_socket_listener_accept_socket_finish(
      rfcomm_listener_, res, NULL, &error);
  if (!socket) {
    g_printerr("\n\nlistener_accept_socket_finish failed %s\n", error->message);
    return;
  }

  sockets_.push_back(socket);

  int fd = g_socket_get_fd(socket);
  uint32_t channel = rfcomm_get_channel(fd);
  char address[18];  // "XX:XX:XX:XX:XX:XX"
  picojson::value::object o;

  rfcomm_get_peer(fd, address);

  o["cmd"] = picojson::value("RFCOMMSocketAccept");
  o["channel"] = picojson::value(static_cast<double>(channel));
  o["socket_fd"] = picojson::value(static_cast<double>(fd));
  o["peer"] = picojson::value(address);

  InternalPostMessage(picojson::value(o));

  GSource *source = g_socket_create_source(socket, G_IO_IN, NULL);

  g_source_set_callback(source, (GSourceFunc)BluetoothInstance::OnSocketHasData,
                        this, NULL);
  g_source_attach(source, NULL);
  g_source_unref(source);
}

void BluetoothInstance::OnServiceAddRecord(GObject* object, GAsyncResult* res) {
  GError* error = 0;
  picojson::value::object o;
  GVariant* result = g_dbus_proxy_call_finish(service_proxy_, res, &error);

  o["cmd"] = picojson::value("");
  o["reply_id"] = picojson::value(callbacks_map_["RFCOMMListen"]);

  if (!result) {
    o["error"] = picojson::value(static_cast<double>(UNKNOWN_ERR));

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

    servers_.push_back(socket);

    g_socket_listener_add_socket(rfcomm_listener_, socket, NULL, NULL);

    g_socket_listener_accept_async(rfcomm_listener_, NULL,
                                   OnListenerAcceptThunk, this);

    g_variant_get(result, "(u)", &handle);

    o["error"] = picojson::value(static_cast<double>(NO_ERROR));
    o["server_fd"] = picojson::value(static_cast<double>(sk));
    o["sdp_handle"] = picojson::value(static_cast<double>(handle));
    o["channel"] = picojson::value(static_cast<double>(rfcomm_get_channel(sk)));

    g_variant_unref(result);
  }

  callbacks_map_.erase("RFCOMMListen");

  InternalPostMessage(picojson::value(o));
}

void BluetoothInstance::HandleRFCOMMListen(const picojson::value& msg) {
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

  char *record = g_strdup_printf(kRFCOMMRecord, uuid.c_str(),
                                 channel, name.c_str());

  g_dbus_proxy_call(service_proxy_, "AddRecord", g_variant_new("(s)", record),
                    G_DBUS_CALL_FLAGS_NONE, -1, all_pending_,
                    OnServiceAddRecordThunk,
                    CancellableWrap(all_pending_, this));
}

void BluetoothInstance::OnDeviceProxyCreated(
    GObject* object, GAsyncResult* res) {
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

  g_dbus_proxy_call(
      device_proxy, "GetProperties", NULL,
      G_DBUS_CALL_FLAGS_NONE, 5000, all_pending_, OnGotDevicePropertiesThunk,
      CancellableWrap(all_pending_, this));

  g_signal_connect(device_proxy, "g-signal",
    G_CALLBACK(BluetoothInstance::OnDeviceSignal), this);
}

void BluetoothInstance::OnGotDeviceProperties(
    GObject* object, GAsyncResult* res) {
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
  InternalPostMessage(v);
}

void BluetoothInstance::HandleSocketWriteData(const picojson::value& msg) {
  int fd = static_cast<int>(msg.get("socket_fd").get<double>());
  auto it = sockets_.begin();
  gssize len = 0;

  for (; it != sockets_.end(); ++it) {
    GSocket *socket = *it;

    if (g_socket_get_fd(socket) == fd) {
      std::string data = msg.get("data").to_str();

      len = g_socket_send(socket, data.c_str(), data.length(), NULL, NULL);
      break;
    }
  }

  picojson::value::object o;
  o["size"] = picojson::value(static_cast<double>(len));

  InternalSetSyncReply(picojson::value(o));
}

void BluetoothInstance::HandleCloseSocket(const picojson::value& msg) {
  int fd = static_cast<int>(msg.get("socket_fd").get<double>());
  std::vector<GSocket*>::iterator it = sockets_.begin();

  for (; it != sockets_.end(); ++it) {
    GSocket *socket = *it;

    if (g_socket_get_fd(socket) == fd) {
      g_socket_close(socket, NULL);
      break;
    }
  }

  picojson::value::object o;
  o["cmd"] = picojson::value("");
  o["reply_id"] = msg.get("reply_id");
  o["error"] = picojson::value(static_cast<double>(NO_ERROR));

  picojson::value v(o);
  InternalPostMessage(v);
}

void BluetoothInstance::OnServiceRemoveRecord(
    GObject* object, GAsyncResult* res) {
  GError* error = 0;
  GVariant* result = g_dbus_proxy_call_finish(service_proxy_, res, &error);
  picojson::value::object o;

  if (!result) {
    o["error"] = picojson::value(static_cast<double>(UNKNOWN_ERR));
  } else {
    o["error"] = picojson::value(static_cast<double>(NO_ERROR));
    g_variant_unref(result);
  }

  o["cmd"] = picojson::value("");
  o["reply_id"] = picojson::value(callbacks_map_["UnregisterServer"]);

  callbacks_map_.erase("UnregisterServer");

  InternalPostMessage(picojson::value(o));
}

void BluetoothInstance::HandleUnregisterServer(const picojson::value& msg) {
  int fd = static_cast<int>(msg.get("server_fd").get<double>());
  uint32_t handle = static_cast<uint32_t>(msg.get("sdp_handle").get<double>());
  std::vector<GSocket*>::iterator it = servers_.begin();

  for (; it != servers_.end(); ++it) {
    GSocket *socket = *it;

    if (g_socket_get_fd(socket) == fd) {
      g_socket_close(socket, NULL);
      break;
    }
  }

  callbacks_map_["UnregisterServer"] = msg.get("reply_id").to_str();

  g_dbus_proxy_call(service_proxy_, "RemoveRecord",
                    g_variant_new("(u)", handle),
                    G_DBUS_CALL_FLAGS_NONE, -1, all_pending_,
                    OnServiceRemoveRecordThunk,
                    CancellableWrap(all_pending_, this));
}

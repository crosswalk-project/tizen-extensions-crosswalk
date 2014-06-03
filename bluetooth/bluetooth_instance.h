// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BLUETOOTH_BLUETOOTH_INSTANCE_H_
#define BLUETOOTH_BLUETOOTH_INSTANCE_H_

#include <gio/gio.h>
#include <map>
#include <string>
#include <vector>

#include "common/extension.h"
#include "common/picojson.h"

#define G_CALLBACK_1(METHOD, SENDER, ARG0)                                     \
  static void METHOD ## Thunk(SENDER sender, ARG0 res, gpointer userdata) {    \
    return reinterpret_cast<BluetoothInstance*>(userdata)->METHOD(sender,      \
                                                                  res);        \
  }                                                                            \
                                                                               \
  void METHOD(SENDER, ARG0);

#define G_CALLBACK_CANCELLABLE_1(METHOD, SENDER, ARG0)                         \
  static void METHOD ## Thunk(SENDER sender, ARG0 res, gpointer userdata) {    \
    ContextCancellable* context =                                              \
        reinterpret_cast<ContextCancellable*>(userdata);                       \
    if (context->cancellable                                                   \
        && g_cancellable_is_cancelled(context->cancellable))                   \
      goto done;                                                               \
    reinterpret_cast<BluetoothInstance*>(context->userdata)->METHOD(sender,    \
                                                                   res);       \
  done:                                                                        \
    delete context;                                                            \
  }                                                                            \
                                                                               \
  void METHOD(SENDER, ARG0);

#define G_CALLBACK_CANCELLABLE_2(METHOD, SENDER, ARG0)                         \
  static void METHOD ## Thunk(SENDER sender, ARG0 res, gpointer userdata) {    \
    OnAdapterPropertySetData* callback_data =                                  \
        reinterpret_cast<OnAdapterPropertySetData*>(userdata);                 \
    if (callback_data->cancellable &&                                          \
        g_cancellable_is_cancelled(callback_data->cancellable)) {              \
      delete callback_data;                                                    \
      return;                                                                  \
    }                                                                          \
    std::string property = callback_data->property;                            \
    reinterpret_cast<BluetoothInstance*>(                                      \
        callback_data->bt_context)->METHOD(property, res);                     \
    delete callback_data;                                                      \
    return;                                                                    \
  }                                                                            \
                                                                               \
  void METHOD(std::string, ARG0);

typedef std::map<std::string, GDBusProxy*> DeviceMap;

namespace picojson {
class value;
}

typedef struct OnAdapterPropertySetData_ {
  GCancellable* cancellable;
  std::string property;
  void* bt_context;
} OnAdapterPropertySetData;

typedef struct ContextCancellable_ {
  GCancellable* cancellable;
  void* userdata;
} ContextCancellable;

static ContextCancellable* CancellableWrap(
    GCancellable *cancellable, void* userdata) {
  ContextCancellable* context = new ContextCancellable();

  context->cancellable = cancellable;
  context->userdata = userdata;

  return context;
}


class BluetoothInstance : public common::Instance {
 public:
  BluetoothInstance();
  ~BluetoothInstance();

 private:
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* msg);

  void PlatformInitialize();

  G_CALLBACK_CANCELLABLE_1(OnAdapterProxyCreated, GObject*, GAsyncResult*);
  G_CALLBACK_CANCELLABLE_1(OnDiscoveryStarted, GObject*, GAsyncResult*);
  G_CALLBACK_CANCELLABLE_1(OnDiscoveryStopped, GObject*, GAsyncResult*);
  G_CALLBACK_CANCELLABLE_1(OnManagerCreated, GObject*, GAsyncResult*);

  void HandleDiscoverDevices(const picojson::value& msg);
  void HandleStopDiscovery(const picojson::value& msg);
  void HandleGetDefaultAdapter(const picojson::value& msg);
  void HandleSetAdapterProperty(const picojson::value& msg);
  void HandleCreateBonding(const picojson::value& msg);
  void HandleDestroyBonding(const picojson::value& msg);
  void HandleRFCOMMListen(const picojson::value& msg);
  void HandleSocketWriteData(const picojson::value& msg);
  void HandleCloseSocket(const picojson::value& msg);
  void HandleUnregisterServer(const picojson::value& msg);

  void InternalPostMessage(picojson::value v);
  void InternalSetSyncReply(picojson::value v);
  void FlushPendingMessages();

  void AdapterInfoToValue(picojson::value::object& o);

  void AdapterSendGetDefaultAdapterReply();

  std::string discover_callback_id_;
  std::string stop_discovery_callback_id_;
  std::map<std::string, std::string> adapter_info_;
  GDBusProxy* adapter_proxy_;

  typedef std::vector<picojson::value> MessageQueue;
  MessageQueue queue_;

  DeviceMap known_devices_;
  bool is_js_context_initialized_;

  std::string default_adapter_reply_id_;
  GCancellable *all_pending_;

#if defined(BLUEZ_5)
  G_CALLBACK_1(OnDBusObjectAdded, GDBusObjectManager*, GDBusObject*);
  G_CALLBACK_1(OnDBusObjectRemoved, GDBusObjectManager*, GDBusObject*);
  G_CALLBACK_1(DeviceFound, GObject*, GAsyncResult*);
  G_CALLBACK_1(KnownDeviceFound, GObject*, GAsyncResult*);

  static void CacheManagedObject(gpointer data, gpointer user_data);
  static void OnPropertiesChanged(GDBusProxy* proxy,
      GVariant* changed_properties, const gchar* const* invalidated_properties,
      gpointer user_data);

  void DeviceRemoved(GDBusObject* object);
  GDBusProxy* CreateDeviceProxy(GAsyncResult* res);

  GDBusObjectManager* object_manager_;
#elif defined(BLUEZ_4)
  G_CALLBACK_CANCELLABLE_1(OnGotDefaultAdapterPath, GObject*, GAsyncResult*);
  G_CALLBACK_CANCELLABLE_1(OnGotAdapterProperties, GObject*, GAsyncResult*);
  G_CALLBACK_CANCELLABLE_1(OnAdapterCreateBonding, GObject*, GAsyncResult*);
  G_CALLBACK_CANCELLABLE_1(OnAdapterDestroyBonding, GObject*, GAsyncResult*);
  G_CALLBACK_CANCELLABLE_1(OnFoundDevice, GObject*, GAsyncResult*);
  G_CALLBACK_CANCELLABLE_2(OnAdapterPropertySet, GObject*, GAsyncResult*);
  G_CALLBACK_CANCELLABLE_1(OnDeviceProxyCreated, GObject*, GAsyncResult*);
  G_CALLBACK_CANCELLABLE_1(OnGotDeviceProperties, GObject*, GAsyncResult*);
  G_CALLBACK_CANCELLABLE_1(OnServiceProxyCreated, GObject*, GAsyncResult*);
  G_CALLBACK_CANCELLABLE_1(OnServiceAddRecord, GObject*, GAsyncResult*);
  G_CALLBACK_1(OnListenerAccept, GObject*, GAsyncResult*);
  G_CALLBACK_CANCELLABLE_1(OnServiceRemoveRecord, GObject*, GAsyncResult*);

  static void OnSignal(GDBusProxy* proxy, gchar* sender_name, gchar* signal,
      GVariant* parameters, gpointer user_data);

  static void OnDeviceSignal(
      GDBusProxy* proxy, gchar* sender_name, gchar* signal,
      GVariant* parameters, gpointer user_data);

  static void OnManagerSignal(
      GDBusProxy* proxy, gchar* sender_name, gchar* signal,
      GVariant* parameters, gpointer user_data);

  static void OnBluetoothServiceAppeared(GDBusConnection* connection,
                                         const char* name,
                                         const char* name_owner,
                                         gpointer user_data);

  static void OnBluetoothServiceVanished(GDBusConnection* connection,
                                         const char* name,
                                         gpointer user_data);

  void AdapterSetPowered(const picojson::value& msg);

  void DeviceFound(std::string address, GVariantIter* properties);

  static gboolean OnSocketHasData(GSocket* client, GIOCondition cond,
                              gpointer user_data);

  GDBusProxy* manager_proxy_;
  std::map<std::string, std::string> callbacks_map_;
  std::map<std::string, std::string> object_path_address_map_;

  GDBusProxy* service_proxy_;
  int pending_listen_socket_;

  std::vector<GSocket*> sockets_;
  std::vector<GSocket*> servers_;

  GSocketListener *rfcomm_listener_;

  guint name_watch_id_;
#endif
};

#endif  // BLUETOOTH_BLUETOOTH_INSTANCE_H_

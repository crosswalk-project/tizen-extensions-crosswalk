// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BLUETOOTH_BLUETOOTH_CONTEXT_H_
#define BLUETOOTH_BLUETOOTH_CONTEXT_H_

#include "common/extension_adapter.h"
#include <map>
#include <string>
#include <vector>
#include <gio/gio.h>

#define G_CALLBACK_1(METHOD, SENDER, ARG0)                                     \
  static void METHOD ## Thunk(SENDER sender, ARG0 res, gpointer userdata) {    \
    return reinterpret_cast<BluetoothContext*>(userdata)->METHOD(sender, res); \
  }                                                                            \
                                                                               \
  void METHOD(SENDER, ARG0);

#define G_CALLBACK_2(METHOD, SENDER, ARG0)                                     \
  static void METHOD ## Thunk(SENDER sender, ARG0 res, gpointer userdata) {    \
    OnAdapterPropertySetData* callback_data =                                  \
        reinterpret_cast<OnAdapterPropertySetData*>(userdata);                 \
    std::string property = callback_data->property;                            \
    reinterpret_cast<BluetoothContext*>(callback_data->bt_context)->METHOD(property,\
        res);                                                                  \
    delete callback_data;                                                      \
    return;                                                                    \
  }                                                                            \
                                                                               \
  void METHOD(std::string, ARG0);

#if defined(GENERIC_DESKTOP)
typedef std::map<std::string, GDBusProxy*> DeviceMap;
#elif defined(TIZEN_MOBILE)
typedef std::map<std::string, GVariantIter*> DeviceMap;
#endif

namespace picojson {
class value;
}

typedef struct OnAdapterPropertySetData_ {
  std::string property;
  void* bt_context;
} OnAdapterPropertySetData;


class BluetoothContext {
 public:
  BluetoothContext(ContextAPI* api);
  ~BluetoothContext();

  // ExtensionAdapter implementation.
  static const char name[];
  static const char* GetJavaScript();
  void HandleMessage(const char* message);
  void HandleSyncMessage(const char* message);

 private:
  void PlatformInitialize();

  G_CALLBACK_1(OnAdapterProxyCreated, GObject*, GAsyncResult*);
  G_CALLBACK_1(OnDiscoveryStarted, GObject*, GAsyncResult*);
  G_CALLBACK_1(OnDiscoveryStopped, GObject*, GAsyncResult*);
  G_CALLBACK_1(OnManagerCreated, GObject*, GAsyncResult*);

  void HandleDiscoverDevices(const picojson::value& msg);
  void HandleStopDiscovery(const picojson::value& msg);
  picojson::value HandleGetDefaultAdapter(const picojson::value& msg);
  void HandleSetAdapterProperty(const picojson::value& msg);

  void PostMessage(picojson::value v);
  void SetSyncReply(picojson::value v);
  void FlushPendingMessages();

  ContextAPI* api_;
  std::string discover_callback_id_;
  std::string stop_discovery_callback_id_;
  std::map<std::string, std::string> adapter_info_;
  GDBusProxy* adapter_proxy_;

  typedef std::vector<picojson::value> MessageQueue;
  MessageQueue queue_;

  DeviceMap known_devices_;
  bool is_js_context_initialized_;

#if defined(GENERIC_DESKTOP)
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
#elif defined(TIZEN_MOBILE)
  G_CALLBACK_1(OnGotDefaultAdapterPath, GObject*, GAsyncResult*);
  G_CALLBACK_1(OnGotAdapterProperties, GObject*, GAsyncResult*);
  G_CALLBACK_2(OnAdapterPropertySet, GObject*, GAsyncResult*);

  static void OnSignal(GDBusProxy* proxy, gchar* sender_name, gchar* signal,
      GVariant* parameters, gpointer user_data);

  void DeviceFound(std::string address, GVariantIter* properties);

  GDBusProxy* manager_proxy_;
  std::map<std::string, std::string> callbacks_map_;
#endif
};

#endif  // BLUETOOTH_BLUETOOTH_CONTEXT_H_

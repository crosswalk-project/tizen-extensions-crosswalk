// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BLUETOOTH_BLUETOOTH_CONTEXT_H_
#define BLUETOOTH_BLUETOOTH_CONTEXT_H_

#define G_CALLBACK_1(METHOD, SENDER, ARG0)                          \
  static void METHOD ## Thunk(SENDER sender, ARG0 res, gpointer userdata) {    \
    return reinterpret_cast<BluetoothContext*>(userdata)->METHOD(sender, res); \
  }                                                                            \
                                                                               \
  void METHOD(SENDER, ARG0);


#include "common/extension_adapter.h"
#include <map>
#include <string>
#include <vector>

#if defined(GENERIC_DESKTOP)
#include <gio/gio.h>
#endif

namespace picojson {
class value;
}

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

  void HandleDiscoverDevices(const picojson::value& msg);
  void HandleStopDiscovery(const picojson::value& msg);
  picojson::value HandleGetDefaultAdapter(const picojson::value& msg);

  ContextAPI* api_;

#if defined(GENERIC_DESKTOP)
  void PostMessage(picojson::value v);
  void SetSyncReply(picojson::value v);
  void FlushPendingMessages();

  G_CALLBACK_1(OnObjectManagerCreated, GObject*, GAsyncResult*);
  G_CALLBACK_1(OnAdapterProxyCreated, GObject*, GAsyncResult*);
  G_CALLBACK_1(OnDiscoveryStarted, GObject*, GAsyncResult*);
  G_CALLBACK_1(OnDiscoveryStopped, GObject*, GAsyncResult*);
  G_CALLBACK_1(DeviceFound, GObject*, GAsyncResult*);
  G_CALLBACK_1(KnownDeviceFound, GObject*, GAsyncResult*);
  G_CALLBACK_1(OnDBusObjectAdded, GDBusObjectManager*, GDBusObject*);
  G_CALLBACK_1(OnDBusObjectRemoved, GDBusObjectManager*, GDBusObject*);

  static void CacheManagedObject(gpointer data, gpointer user_data);
  static void OnPropertiesChanged(GDBusProxy* proxy,
      GVariant* changed_properties, const gchar* const* invalidated_properties,
      gpointer user_data);

  void DeviceRemoved(GDBusObject* object);
  GDBusProxy* CreateDeviceProxy(GAsyncResult* res);

  std::string discover_callback_id_;
  std::string stop_discovery_callback_id_;

  std::map<std::string, std::string> adapter_info_;

  GDBusProxy* adapter_proxy_;
  GDBusObjectManager* object_manager_;
  std::map<std::string, GDBusProxy*> known_devices_;

  typedef std::vector<picojson::value> MessageQueue;
  MessageQueue queue_;

  bool is_js_context_initialized_;
#endif
};

#endif  // BLUETOOTH_BLUETOOTH_CONTEXT_H_

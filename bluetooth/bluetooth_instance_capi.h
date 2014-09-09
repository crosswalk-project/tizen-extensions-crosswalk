// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BLUETOOTH_BLUETOOTH_INSTANCE_CAPI_H_
#define BLUETOOTH_BLUETOOTH_INSTANCE_CAPI_H_

#include <bluetooth.h>
#include <glib.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "common/extension.h"
#include "common/picojson.h"

#define LOG_ERR(msg) std::cerr << "[Error] " << msg << std::endl

// Macros interfacing with C code from Bluetooth API.
#define CAPI(fnc)                                                              \
  do {                                                                         \
    int _er = (fnc);                                                           \
    if (_er != BT_ERROR_NONE) {                                                \
      LOG_ERR(#fnc " failed");                                                 \
    }                                                                          \
  } while (0)

// same CAPI macro providing error code
#define CAPI_ERR(fnc, _er)                                                     \
  do {                                                                         \
    _er = (fnc);                                                               \
    if (_er != BT_ERROR_NONE) {                                                \
      LOG_ERR(#fnc " failed");                                                 \
    }                                                                          \
  } while (0)

namespace picojson {

class value;

}  // namespace picojson

class BluetoothInstance : public common::Instance {
 public:
  BluetoothInstance();
  ~BluetoothInstance();

 private:
  virtual void Initialize();
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* msg);

  void InitializeAdapter();
  void UninitializeAdapter();

  void HandleDiscoverDevices(const picojson::value& msg);
  void HandleStopDiscovery(const picojson::value& msg);
  void HandleGetDefaultAdapter(const picojson::value& msg);
  void HandleSetAdapterProperty(const picojson::value& msg);
  void HandleCreateBonding(const picojson::value& msg);
  void HandleDestroyBonding(const picojson::value& msg);
  void HandleRFCOMMListen(const picojson::value& msg);
  void HandleConnectToService(const picojson::value& msg);
  void HandleSocketWriteData(const picojson::value& msg);
  void HandleCloseSocket(const picojson::value& msg);
  void HandleUnregisterServer(const picojson::value& msg);
  void HandleRegisterSinkApp(const picojson::value& msg);
  void HandleUnregisterSinkApp(const picojson::value& msg);
  void HandleConnectToSource(const picojson::value& msg);
  void HandleDisconnectSource(const picojson::value& msg);
  void HandleSendHealthData(const picojson::value& msg);

  void InternalPostMessage(picojson::value v);
  void InternalSetSyncReply(picojson::value v);
  void FlushPendingMessages();

  static gboolean GetDefaultAdapter(gpointer user_data);

  static void OnStateChanged(int result, bt_adapter_state_e adapter_state,
      void* user_data);

  static void OnNameChanged(char* name, void* user_data);

  static void OnVisibilityChanged(int result,
      bt_adapter_visibility_mode_e visibility_mode, void* user_data);

  static void OnDiscoveryStateChanged(int result,
      bt_adapter_device_discovery_state_e discovery_state,
      bt_adapter_device_discovery_info_s* discovery_info, void* user_data);

  static void OnBondCreated(int result, bt_device_info_s* device_info,
      void* user_data);

  static void OnBondDestroyed(int result, char* remote_address,
      void* user_data);

  static bool OnKnownBondedDevice(bt_device_info_s* device_info,
      void* user_data);

  static void OnSocketConnected(int result,
      bt_socket_connection_state_e connection_state,
      bt_socket_connection_s* connection, void* user_data);

  static void OnSocketHasData(bt_socket_received_data_s* data, void* user_data);

  static void OnHdpConnected(int result, const char* remote_address,
      const char* app_id, bt_hdp_channel_type_e type, unsigned int channel,
      void* user_data);

  static void OnHdpDisconnected(int result, const char* remote_address,
      unsigned int channel, void* user_data);

  static void OnHdpDataReceived(unsigned int channel, const char* data,
      unsigned int size, void* user_data);

  // Map JS reply_id to a C API callback
  std::map<std::string, std::string> callbacks_id_map_;

  std::map<int, bool> socket_connected_map_;

  typedef std::vector<picojson::value> MessageQueue;
  MessageQueue queue_;

  bool is_js_context_initialized_;
  bool adapter_enabled_;
  bool js_reply_needed_;
  bool stop_discovery_from_js_;
};

#endif  // BLUETOOTH_BLUETOOTH_INSTANCE_CAPI_H_

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(TIZEN_MOBILE)
#include <bluetooth.h>
#endif

#include "bluetooth/bluetooth_context.h"
#include "common/picojson.h"

int32_t XW_Initialize(XW_Extension extension, XW_GetInterface get_interface) {
#if defined(TIZEN_MOBILE)
  int init = bt_initialize();
  if (init != BT_ERROR_NONE)
    g_printerr("\n\nCouldn't initialize Bluetooth module.");
#endif

  return ExtensionAdapter<BluetoothContext>::Initialize(extension,
                                                        get_interface);
}

BluetoothContext::BluetoothContext(ContextAPI* api)
    : api_(api) {
  PlatformInitialize();
}

const char BluetoothContext::name[] = "tizen.bluetooth";

const char* BluetoothContext::entry_points[] = { NULL };

extern const char kSource_bluetooth_api[];

const char* BluetoothContext::GetJavaScript() {
  return kSource_bluetooth_api;
}

void BluetoothContext::HandleMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "DiscoverDevices")
    HandleDiscoverDevices(v);
  else if (cmd == "StopDiscovery")
    HandleStopDiscovery(v);
  else if (cmd == "SetAdapterProperty")
    HandleSetAdapterProperty(v);
  else if (cmd == "CreateBonding")
    HandleCreateBonding(v);
  else if (cmd == "DestroyBonding")
    HandleDestroyBonding(v);
  else if (cmd == "RFCOMMListen")
    HandleRFCOMMListen(v);
  else if (cmd == "CloseSocket")
    HandleCloseSocket(v);
  else if (cmd == "UnregisterServer")
    HandleUnregisterServer(v);
}

void BluetoothContext::HandleSyncMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring Sync message.\n";
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "GetDefaultAdapter")
    HandleGetDefaultAdapter(v);
  else if (cmd == "SocketWriteData")
    HandleSocketWriteData(v);
}

void BluetoothContext::HandleDiscoverDevices(const picojson::value& msg) {
  discover_callback_id_ = msg.get("reply_id").to_str();
  if (adapter_proxy_) {
    g_dbus_proxy_call(
        adapter_proxy_, "StartDiscovery", NULL,
        G_DBUS_CALL_FLAGS_NONE, 20000, NULL, OnDiscoveryStartedThunk,
        CancellableWrap(all_pending_, this));
  }
}

void BluetoothContext::HandleStopDiscovery(const picojson::value& msg) {
  stop_discovery_callback_id_ = msg.get("reply_id").to_str();
  if (adapter_proxy_) {
    g_dbus_proxy_call(
        adapter_proxy_, "StopDiscovery", NULL,
        G_DBUS_CALL_FLAGS_NONE, 20000, NULL, OnDiscoveryStoppedThunk,
        CancellableWrap(all_pending_, this));
  }
}

void BluetoothContext::OnDiscoveryStarted(GObject*, GAsyncResult* res) {
  GError* error = 0;

  GVariant* result = g_dbus_proxy_call_finish(adapter_proxy_, res, &error);

  picojson::value::object o;
  o["cmd"] = picojson::value("");
  o["reply_id"] = picojson::value(discover_callback_id_);

  int errorCode = 0;
  if (!result) {
    g_printerr("Error discovering: %s\n", error->message);
    g_error_free(error);
    errorCode = 1;
  }

  o["error"] = picojson::value(static_cast<double>(errorCode));

  picojson::value v(o);
  PostMessage(v);

  if (result)
    g_variant_unref(result);
}

void BluetoothContext::OnDiscoveryStopped(GObject* source, GAsyncResult* res) {
  GError* error = 0;
  GVariant* result = g_dbus_proxy_call_finish(adapter_proxy_, res, &error);

  if (!result) {
    g_printerr("Error discovering: %s\n", error->message);
    g_error_free(error);
  }

  if (stop_discovery_callback_id_.empty())
    return;

  picojson::value::object o;
  o["cmd"] = picojson::value("");
  o["reply_id"] = picojson::value(stop_discovery_callback_id_);
  stop_discovery_callback_id_.clear();
  o["error"] = picojson::value(static_cast<double>(0));
  picojson::value v(o);
  PostMessage(v);
}

void BluetoothContext::FlushPendingMessages() {
  // Flush previous pending messages.
  if (!queue_.empty()) {
    MessageQueue::iterator it;
    for (it = queue_.begin(); it != queue_.end(); ++it)
      api_->PostMessage((*it).serialize().c_str());
  }
}

void BluetoothContext::AdapterInfoToValue(picojson::value::object& o) {
  o["cmd"] = picojson::value("");

  if (adapter_info_.empty()) {
    o["error"] = picojson::value(static_cast<double>(1));
    return;
  }

  o["name"] = picojson::value(adapter_info_["Name"]);
  o["address"] = picojson::value(adapter_info_["Address"]);

  bool powered = (adapter_info_["Powered"] == "true") ? true : false;
  o["powered"] = picojson::value(powered);

  bool visible = (adapter_info_["Discoverable"] == "true") ? true : false;
  o["visible"] = picojson::value(visible);

  o["error"] = picojson::value(static_cast<double>(0));
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
  api_->SetSyncReply(v.serialize().c_str());

  FlushPendingMessages();
}

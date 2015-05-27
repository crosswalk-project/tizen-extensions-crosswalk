// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bluetooth/bluetooth_instance.h"

#include "common/picojson.h"
#include "tizen/tizen.h"

BluetoothInstance::BluetoothInstance() {
  PlatformInitialize();
}

void BluetoothInstance::HandleMessage(const char* msg) {
  picojson::value v;

  std::string err;
  picojson::parse(v, msg, msg + strlen(msg), &err);
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

void BluetoothInstance::HandleSyncMessage(const char* msg) {
  picojson::value v;

  std::string err;
  picojson::parse(v, msg, msg + strlen(msg), &err);
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

void BluetoothInstance::HandleDiscoverDevices(const picojson::value& msg) {
  discover_callback_id_ = msg.get("reply_id").to_str();
  if (adapter_proxy_) {
    g_dbus_proxy_call(
        adapter_proxy_, "StartDiscovery", NULL,
        G_DBUS_CALL_FLAGS_NONE, 20000, NULL, OnDiscoveryStartedThunk,
        CancellableWrap(all_pending_, this));
  }
}

void BluetoothInstance::HandleStopDiscovery(const picojson::value& msg) {
  stop_discovery_callback_id_ = msg.get("reply_id").to_str();
  if (adapter_proxy_) {
    g_dbus_proxy_call(
        adapter_proxy_, "StopDiscovery", NULL,
        G_DBUS_CALL_FLAGS_NONE, 20000, NULL, OnDiscoveryStoppedThunk,
        CancellableWrap(all_pending_, this));
  }
}

void BluetoothInstance::OnDiscoveryStarted(GObject*, GAsyncResult* res) {
  GError* error = 0;

  GVariant* result = g_dbus_proxy_call_finish(adapter_proxy_, res, &error);

  picojson::value::object o;
  o["cmd"] = picojson::value("");
  o["reply_id"] = picojson::value(discover_callback_id_);

  int errorCode = NO_ERROR;
  if (!result) {
    g_printerr("Error discovering: %s\n", error->message);
    g_error_free(error);
    errorCode = UNKNOWN_ERR;
  }

  o["error"] = picojson::value(static_cast<double>(errorCode));

  picojson::value v(o);
  InternalPostMessage(v);

  if (result)
    g_variant_unref(result);
}

void BluetoothInstance::OnDiscoveryStopped(GObject* source, GAsyncResult* res) {
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
  o["error"] = picojson::value(static_cast<double>(NO_ERROR));
  picojson::value v(o);
  InternalPostMessage(v);
}

void BluetoothInstance::FlushPendingMessages() {
  // Flush previous pending messages.
  if (!queue_.empty()) {
    MessageQueue::iterator it;
    for (it = queue_.begin(); it != queue_.end(); ++it)
      PostMessage((*it).serialize().c_str());
  }
}

void BluetoothInstance::AdapterInfoToValue(picojson::value::object& o) {
  o["cmd"] = picojson::value("");

  // Sending a dummy adapter, so JS can call setPowered(true) on it.
  if (adapter_info_.empty()) {
    o["address"] = picojson::value("00:00:00:00:00");
    return;
  }

  o["name"] = picojson::value(adapter_info_["Name"]);
  o["address"] = picojson::value(adapter_info_["Address"]);

  bool powered = (adapter_info_["Powered"] == "true");
  o["powered"] = picojson::value(powered);

  bool visible = (adapter_info_["Discoverable"] == "true");
  o["visible"] = picojson::value(visible);

  o["error"] = picojson::value(static_cast<double>(NO_ERROR));
}

void BluetoothInstance::AdapterSendGetDefaultAdapterReply() {
  if (default_adapter_reply_id_.empty())
    return;

  picojson::value::object o;
  o["reply_id"] = picojson::value(default_adapter_reply_id_);
  AdapterInfoToValue(o);

  // This is the JS API entry point, so we should clean our message queue
  // on the next PostMessage call.
  if (!is_js_context_initialized_)
    is_js_context_initialized_ = true;

  InternalSetSyncReply(picojson::value(o));

  default_adapter_reply_id_.clear();
}

void BluetoothInstance::InternalPostMessage(picojson::value v) {
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
  PostMessage(v.serialize().c_str());
}

void BluetoothInstance::InternalSetSyncReply(picojson::value v) {
  SendSyncReply(v.serialize().c_str());

  FlushPendingMessages();
}

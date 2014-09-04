// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bluetooth/bluetooth_instance_capi.h"

#include "common/picojson.h"

namespace {

inline const char* BoolToString(bool b) {
  return b ? "true" : "false";
}

}  // anonymous namespace

BluetoothInstance::BluetoothInstance()
    : is_js_context_initialized_(false),
      adapter_enabled_(false),
      js_reply_needed_(false),
      stop_discovery_from_js_(false) {
}

BluetoothInstance::~BluetoothInstance() {
  UninitializeAdapter();
  CAPI(bt_deinitialize());
}

void BluetoothInstance::Initialize() {
  CAPI(bt_initialize());
  InitializeAdapter();
}

void BluetoothInstance::HandleMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    LOG_ERR("Ignoring message");
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
  else if (cmd == "ConnectToService")
    HandleConnectToService(v);
  else if (cmd == "CloseSocket")
    HandleCloseSocket(v);
  else if (cmd == "UnregisterServer")
    HandleUnregisterServer(v);
  else if (cmd == "RegisterSinkApp")
    HandleRegisterSinkApp(v);
  else if (cmd == "UnregisterSinkApp")
    HandleUnregisterSinkApp(v);
  else if (cmd == "ConnectToSource")
    HandleConnectToSource(v);
  else if (cmd == "DisconnectSource")
    HandleDisconnectSource(v);
  else if (cmd == "SendHealthData")
    HandleSendHealthData(v);
}

void BluetoothInstance::HandleSyncMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    LOG_ERR("Ignoring Sync message.");
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "GetDefaultAdapter")
    HandleGetDefaultAdapter(v);
  else if (cmd == "SocketWriteData")
    HandleSocketWriteData(v);
}

void BluetoothInstance::OnStateChanged(int result,
    bt_adapter_state_e adapter_state, void* user_data) {
  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);
  if (!obj) {
    LOG_ERR("user_data is NULL");
    return;
  }

  obj->adapter_enabled_ = (adapter_state == BT_ADAPTER_ENABLED) ? true : false;

  if (obj->js_reply_needed_) {
    // FIXME(clecou) directly call 'GetDefaultAdapter' once NTB is integrated.
    // After testing, 100 ms is necessary to really get a powered adapter.
    g_timeout_add(100, obj->GetDefaultAdapter, obj);
    return;
  }

  picojson::value::object o;
  o["cmd"] = picojson::value("AdapterUpdated");
  o["error"] = picojson::value(result != 0);
  o["Powered"] = picojson::value(BoolToString(obj->adapter_enabled_));
  o["reply_id"] = picojson::value(obj->callbacks_id_map_["Powered"]);
  obj->InternalPostMessage(picojson::value(o));
  obj->callbacks_id_map_.erase("Powered");
}

void BluetoothInstance::OnNameChanged(char* name, void* user_data) {
  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);
  if (!obj) {
    LOG_ERR("user_data is NULL");
    return;
  }

  picojson::value::object o;

  o["cmd"] = picojson::value("AdapterUpdated");
  o["error"] = picojson::value(static_cast<double>(0));
  o["Name"] = picojson::value(name);
  o["reply_id"] = picojson::value(obj->callbacks_id_map_["Name"]);
  obj->InternalPostMessage(picojson::value(o));
  obj->callbacks_id_map_.erase("Name");
}

void BluetoothInstance::OnVisibilityChanged(int result,
    bt_adapter_visibility_mode_e visibility_mode, void* user_data) {
  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);
  if (!obj) {
    LOG_ERR("user_data is NULL");
    return;
  }

  picojson::value::object o;

  const char* visible =
      (visibility_mode == BT_ADAPTER_VISIBILITY_MODE_NON_DISCOVERABLE) ?
          "false" : "true";

  o["cmd"] = picojson::value("AdapterUpdated");
  o["Discoverable"] = picojson::value(visible);
  o["reply_id"] = picojson::value(obj->callbacks_id_map_["Discoverable"]);
  o["error"] = picojson::value(result != 0);
  obj->InternalPostMessage(picojson::value(o));
  obj->callbacks_id_map_.erase("Discoverable");
}

void BluetoothInstance::OnDiscoveryStateChanged(int result,
    bt_adapter_device_discovery_state_e discovery_state,
    bt_adapter_device_discovery_info_s* discovery_info, void* user_data) {
  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);
  if (!obj) {
    LOG_ERR("user_data is NULL");
    return;
  }

  picojson::value::object o;

  switch (discovery_state) {
    case BT_ADAPTER_DEVICE_DISCOVERY_STARTED: {
      o["cmd"] = picojson::value("");
      o["reply_id"] = picojson::value(obj->callbacks_id_map_["StartDiscovery"]);
      o["error"] = picojson::value(result != 0);
      obj->InternalPostMessage(picojson::value(o));
      obj->callbacks_id_map_.erase("StartDiscovery");
      break;
    }
    case BT_ADAPTER_DEVICE_DISCOVERY_FINISHED: {
      if (obj->stop_discovery_from_js_) {
        o["cmd"] = picojson::value("");
        o["reply_id"] =
            picojson::value(obj->callbacks_id_map_["StopDiscovery"]);
        o["error"] = picojson::value(result != 0);
      } else {
        // discovery stop was not initiated by JS. It was done by a timeout...
        o["cmd"] = picojson::value("DiscoveryFinished");
      }
      obj->InternalPostMessage(picojson::value(o));
      obj->callbacks_id_map_.erase("StopDiscovery");
      obj->stop_discovery_from_js_ = false;
      break;
    }
    case BT_ADAPTER_DEVICE_DISCOVERY_FOUND: {
      o["Alias"] = picojson::value(discovery_info->remote_name);
      o["Address"] = picojson::value(discovery_info->remote_address);

      int major = discovery_info->bt_class.major_device_class;
      int minor = discovery_info->bt_class.minor_device_class;
      int service_class = discovery_info->bt_class.major_service_class_mask;
      o["ClassMajor"] = picojson::value(static_cast<double>(major));
      o["ClassMinor"] = picojson::value(static_cast<double>(minor));
      o["ClassService"] = picojson::value(static_cast<double>(service_class));

      picojson::array uuids;
      for (int i = 0; i < discovery_info->service_count; i++)
        uuids.push_back(picojson::value(discovery_info->service_uuid[i]));

      o["UUIDs"] = picojson::value(uuids);

      bool paired = false;
      bool trusted = false;
      bool connected = false;

      if (discovery_info->is_bonded) {
        bt_device_info_s* device_info = NULL;
        CAPI(bt_adapter_get_bonded_device_info(discovery_info->remote_address,
                                               &device_info));
        if (!device_info)
          LOG_ERR("device_info is NULL");

        if (!device_info->is_bonded)
          LOG_ERR("remote device should be bonded!");

        paired = true;
        trusted = device_info->is_authorized;
        connected = device_info->is_connected;
        CAPI(bt_adapter_free_device_info(device_info));
      }

      o["Paired"] = picojson::value(BoolToString(paired));
      o["Trusted"] = picojson::value(BoolToString(trusted));
      o["Connected"] = picojson::value(BoolToString(connected));

      o["cmd"] = picojson::value("DeviceFound");
      o["found_on_discovery"] = picojson::value(true);

      obj->InternalPostMessage(picojson::value(o));
      break;
    }
#ifdef NTB
    case BT_ADAPTER_DEVICE_DISCOVERY_REMOVED: {
      o["Address"] = picojson::value(discovery_info->remote_address);
      o["cmd"] = picojson::value("DeviceRemoved");
      obj->InternalPostMessage(picojson::value(o));
      break;
    }
#endif
    default:
      LOG_ERR("Unknown discovery state callback!");
      break;
  }
}

bool BluetoothInstance::OnKnownBondedDevice(bt_device_info_s* device_info,
    void* user_data) {
  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);
  if (!obj) {
    LOG_ERR("user_data is NULL!");
    return false;
  }
  if (!device_info) {
    LOG_ERR("device_info is NULL!");
    return false;
  }

  picojson::value::object o;
  char* alias = device_info->remote_name;
  o["Alias"] = picojson::value(alias);

  char* address = device_info->remote_address;
  o["Address"] = picojson::value(address);

  int major = device_info->bt_class.major_device_class;
  int minor = device_info->bt_class.minor_device_class;
  int service_class = device_info->bt_class.major_service_class_mask;
  o["ClassMajor"] = picojson::value(static_cast<double>(major));
  o["ClassMinor"] = picojson::value(static_cast<double>(minor));
  o["ClassService"] = picojson::value(static_cast<double>(service_class));

  // parse UUIDs supported by remote device
  picojson::array uuids;
  for (int i = 0; i < device_info->service_count; i++)
    uuids.push_back(picojson::value(device_info->service_uuid[i]));

  o["UUIDs"] = picojson::value(uuids);
  o["Paired"] = picojson::value(BoolToString(device_info->is_bonded));
  o["Trusted"] = picojson::value(BoolToString(device_info->is_authorized));
  o["Connected"] = picojson::value(BoolToString(device_info->is_connected));
  o["reply_id"] = picojson::value("");
  o["cmd"] = picojson::value("BondedDevice");
  obj->InternalPostMessage(picojson::value(o));
  return true;
}

void BluetoothInstance::OnBondCreated(int result, bt_device_info_s* device_info,
    void* user_data) {
  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);
  if (!obj) {
    LOG_ERR("user_data is NULL!");
    return;
  }
  if (!device_info) {
    LOG_ERR("device_info is NULL!");
    return;
  }

  picojson::value::object o;
  o["cmd"] = picojson::value("");
  o["reply_id"] = picojson::value(obj->callbacks_id_map_["CreateBonding"]);
  o["capi"] = picojson::value(true);
  o["error"] = picojson::value(result != 0);
  obj->InternalPostMessage(picojson::value(o));
  obj->callbacks_id_map_.erase("CreateBonding");
}

void BluetoothInstance::OnBondDestroyed(int result, char* remote_address,
    void* user_data) {
  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);

  if (!obj) {
    LOG_ERR("user_data is NULL!");
    return;
  }

  if (!remote_address) {
    LOG_ERR("remote_address is NULL!");
    return;
  }
  picojson::value::object o;
  o["cmd"] = picojson::value("");
  o["reply_id"] = picojson::value(obj->callbacks_id_map_["DestroyBonding"]);
  o["capi"] = picojson::value(true);
  o["error"] = picojson::value(result != 0);
  obj->InternalPostMessage(picojson::value(o));
  obj->callbacks_id_map_.erase("DestroyBonding");
}

void BluetoothInstance::OnSocketConnected(int result,
    bt_socket_connection_state_e connection_state,
    bt_socket_connection_s* connection,
    void* user_data) {
  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);
  if (!obj) {
    LOG_ERR("user_data is NULL!");
    return;
  }
  if (!connection) {
    LOG_ERR("connection is NULL!");
    return;
  }

  picojson::value::object o;
  o["error"] = picojson::value(result != 0);

  if (connection_state == BT_SOCKET_CONNECTED &&
      connection->local_role == BT_SOCKET_SERVER) {
    o["cmd"] = picojson::value("RFCOMMSocketAccept");
    o["uuid"] = picojson::value(connection->service_uuid);
    o["socket_fd"] =
        picojson::value(static_cast<double>(connection->socket_fd));
    o["peer"] = picojson::value(connection->remote_address);
    obj->socket_connected_map_[connection->socket_fd] = true;

  } else if (connection_state == BT_SOCKET_CONNECTED &&
             connection->local_role == BT_SOCKET_CLIENT) {
    o["cmd"] = picojson::value("");
    o["reply_id"] =
        picojson::value(obj->callbacks_id_map_["ConnectToService"]);
    obj->callbacks_id_map_.erase("ConnectToService");

    o["uuid"] = picojson::value(connection->service_uuid);
    o["socket_fd"] =
        picojson::value(static_cast<double>(connection->socket_fd));
    o["peer"] = picojson::value(connection->remote_address);
    obj->socket_connected_map_[connection->socket_fd] = true;

  } else if (connection_state == BT_SOCKET_DISCONNECTED) {
      o["cmd"] = picojson::value("");
      o["reply_id"] =
          picojson::value(obj->callbacks_id_map_["RFCOMMsocketDestroy"]);
      obj->callbacks_id_map_.erase("RFCOMMsocketDestroy");
      o["socket_fd"] =
          picojson::value(static_cast<double>(connection->socket_fd));
      obj->socket_connected_map_[connection->socket_fd] = false;
  } else {
    LOG_ERR("Unknown role!");
    return;
  }
  obj->InternalPostMessage(picojson::value(o));
}

void BluetoothInstance::OnSocketHasData(bt_socket_received_data_s* data,
                                        void* user_data) {
  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);
  if (!obj) {
    LOG_ERR("user_data is NULL");
    return;
  }
  if (!data) {
    LOG_ERR("data is NULL");
    return;
  }
  picojson::value::object o;
  o["cmd"] = picojson::value("SocketHasData");
  o["socket_fd"] = picojson::value(static_cast<double>(data->socket_fd));
  o["data"] = picojson::value(static_cast<std::string>(data->data));
  obj->InternalPostMessage(picojson::value(o));
}

void BluetoothInstance::OnHdpConnected(int result, const char* remote_address,
    const char* app_id, bt_hdp_channel_type_e type, unsigned int channel,
    void* user_data) {
  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);
  if (!obj) {
    LOG_ERR("user_data is NULL");
    return;
  }

  picojson::value::object o;
  o["error"] = picojson::value(result != 0);
  o["cmd"] = picojson::value("");
  o["reply_id"] = picojson::value(obj->callbacks_id_map_["ConnectToSource"]);
  obj->callbacks_id_map_.erase("ConnectToSource");
  o["address"] = picojson::value(remote_address);
  o["app_id"] = picojson::value(app_id);
  o["channel_type"] = picojson::value(static_cast<double>(type));
  o["channel"] = picojson::value(static_cast<double>(channel));
  o["connected"] = picojson::value("true");
  obj->InternalPostMessage(picojson::value(o));
}

void BluetoothInstance::OnHdpDisconnected(int result,
    const char* remote_address, unsigned int channel, void* user_data) {
  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);
  if (!obj) {
    LOG_ERR("user_data is NULL");
    return;
  }

  picojson::value::object o;
  o["error"] = picojson::value(result != 0);
  o["cmd"] = picojson::value("");
  o["reply_id"] = picojson::value(obj->callbacks_id_map_["DisconnectSource"]);
  obj->callbacks_id_map_.erase("DisconnectSource");
  o["address"] = picojson::value(remote_address);
  o["channel"] = picojson::value(static_cast<double>(channel));
  o["connected"] = picojson::value("false");
  obj->InternalPostMessage(picojson::value(o));
}

void BluetoothInstance::OnHdpDataReceived(unsigned int channel,
    const char* data, unsigned int size, void* user_data) {
  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);
  if (!obj) {
    LOG_ERR("user_data is NULL");
    return;
  }

  picojson::value::object o;
  o["reply_id"] = picojson::value(obj->callbacks_id_map_["SendHealthData"]);
  obj->callbacks_id_map_.erase("SendHealthData");
  o["channel"] = picojson::value(static_cast<double>(channel));
  o["data"] = picojson::value(data);
  o["size"] = picojson::value(static_cast<double>(size));
  obj->InternalPostMessage(picojson::value(o));
}

gboolean BluetoothInstance::GetDefaultAdapter(gpointer user_data) {
  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);
  if (!obj) {
    LOG_ERR("user_data is NULL");
    return TRUE;
  }

  picojson::value::object o;

  char* name = NULL;
  CAPI(bt_adapter_get_name(&name));
  if (!name)
    return TRUE;
  o["name"] = picojson::value(name);

  char* address = NULL;
  CAPI(bt_adapter_get_address(&address));
  if (!address)
    return TRUE;
  o["address"] = picojson::value(address);

  bool powered, visible = false;

  if (obj->adapter_enabled_) {
    powered = true;

  bt_adapter_visibility_mode_e mode =
      BT_ADAPTER_VISIBILITY_MODE_NON_DISCOVERABLE;

  CAPI(bt_adapter_get_visibility(&mode, NULL));
  visible = (mode > 0) ? true : false;
  }
  o["powered"] = picojson::value(powered);
  o["visible"] = picojson::value(visible);

  // This is the JS API entry point, so we should clean our message queue
  // on the next PostMessage call.
  if (!obj->is_js_context_initialized_)
    obj->is_js_context_initialized_ = true;

  obj->InternalSetSyncReply(picojson::value(o));

  // Retrieve already bonded devices linked to the adapter in order to
  // fill known_devices array on javascript side.
  CAPI(bt_adapter_foreach_bonded_device(OnKnownBondedDevice, obj));

  obj->js_reply_needed_ = false;

  return FALSE;
}

void BluetoothInstance::InitializeAdapter() {
  // register C API bluetooth callbacks
  CAPI(bt_adapter_set_state_changed_cb(OnStateChanged, this));
  CAPI(bt_adapter_set_name_changed_cb(OnNameChanged, this));
  CAPI(bt_adapter_set_visibility_mode_changed_cb(OnVisibilityChanged, this));
  CAPI(bt_adapter_set_device_discovery_state_changed_cb(OnDiscoveryStateChanged,
                                                        this));
  CAPI(bt_device_set_bond_created_cb(OnBondCreated, this));
  CAPI(bt_device_set_bond_destroyed_cb(OnBondDestroyed, this));

  CAPI(bt_socket_set_connection_state_changed_cb(OnSocketConnected, this));
  CAPI(bt_socket_set_data_received_cb(OnSocketHasData, this));

  CAPI(bt_hdp_set_connection_state_changed_cb(OnHdpConnected, OnHdpDisconnected,
      this));
  CAPI(bt_hdp_set_data_received_cb(OnHdpDataReceived, this));

  bt_adapter_state_e state = BT_ADAPTER_DISABLED;
  CAPI(bt_adapter_get_state(&state));

  // Most of the C API functions require as precondition to previously had
  // called bt_adapter_enable(). So if adapter is turned OFF, we enable it.
  if (state == BT_ADAPTER_DISABLED) {
    CAPI(bt_adapter_enable());
  } else {
    adapter_enabled_ = true;
  }
}

void BluetoothInstance::UninitializeAdapter() {
  // unregister C API bluetooth callbacks
  CAPI(bt_adapter_unset_state_changed_cb());
  CAPI(bt_adapter_unset_name_changed_cb());
  CAPI(bt_adapter_unset_visibility_mode_changed_cb());
  CAPI(bt_adapter_unset_device_discovery_state_changed_cb());
  CAPI(bt_device_unset_bond_created_cb());
  CAPI(bt_device_unset_bond_destroyed_cb());
  CAPI(bt_socket_unset_connection_state_changed_cb());
  CAPI(bt_socket_unset_data_received_cb());
  CAPI(bt_hdp_unset_connection_state_changed_cb());
  CAPI(bt_hdp_unset_data_received_cb());
}

void BluetoothInstance::HandleGetDefaultAdapter(const picojson::value& msg) {
  if (!adapter_enabled_) {
    js_reply_needed_ = true;
    return;
  }

  GetDefaultAdapter(this);
}

void BluetoothInstance::HandleSetAdapterProperty(const picojson::value& msg) {
  picojson::value::object o;
  int error = 0;
  std::string property = msg.get("property").to_str();
  callbacks_id_map_[property] = msg.get("reply_id").to_str();

  if (property == "Powered") {
    bool power = msg.get("value").get<bool>();
    if (power)
      CAPI(bt_adapter_enable());
    else
      CAPI(bt_adapter_disable());
    return;
  }

  if (property == "Name") {
    std::string name = msg.get("value").to_str();
    CAPI_ERR(bt_adapter_set_name(name.c_str()), error);
    if (error != BT_ERROR_NONE) {
      o["error"] = picojson::value(static_cast<double>(2));
      if (error == BT_ERROR_INVALID_PARAMETER)
        o["error"] = picojson::value(static_cast<double>(1));
      o["cmd"] = picojson::value("");
      o["reply_id"] = msg.get("reply_id");
      InternalPostMessage(picojson::value(o));
      callbacks_id_map_.erase("Name");
    }
    return;
  }

  if (property == "Discoverable") {
    bool visible = msg.get("value").get<bool>();
    int timeout = static_cast<int>(msg.get("timeout").get<double>());

    bt_adapter_visibility_mode_e discoverable_mode =
        BT_ADAPTER_VISIBILITY_MODE_NON_DISCOVERABLE;
    if (visible) {
      if (timeout == 0)
        discoverable_mode = BT_ADAPTER_VISIBILITY_MODE_GENERAL_DISCOVERABLE;
      else
        discoverable_mode = BT_ADAPTER_VISIBILITY_MODE_LIMITED_DISCOVERABLE;
    }
    CAPI_ERR(bt_adapter_set_visibility(discoverable_mode, timeout), error);
    if (error != BT_ERROR_NONE) {
      o["error"] = picojson::value(static_cast<double>(2));
      if (error == BT_ERROR_INVALID_PARAMETER)
        o["error"] = picojson::value(static_cast<double>(1));
      o["cmd"] = picojson::value("");
      o["reply_id"] = msg.get("reply_id");
      InternalPostMessage(picojson::value(o));
      callbacks_id_map_.erase("Discoverable");
    }
  }
}

void BluetoothInstance::HandleDiscoverDevices(const picojson::value& msg) {
  callbacks_id_map_["StartDiscovery"] = msg.get("reply_id").to_str();
  CAPI(bt_adapter_start_device_discovery());
}

void BluetoothInstance::HandleStopDiscovery(const picojson::value& msg) {
  callbacks_id_map_["StopDiscovery"] = msg.get("reply_id").to_str();

  bool is_discovering = false;
  CAPI(bt_adapter_is_discovering(&is_discovering));
  if (!is_discovering) {
    picojson::value::object o;
    o["cmd"] = picojson::value("");
    o["reply_id"] = picojson::value(callbacks_id_map_["StopDiscovery"]);
    callbacks_id_map_.erase("StopDiscovery");
    InternalPostMessage(picojson::value(o));
    return;
  }
  stop_discovery_from_js_ = true;
  CAPI(bt_adapter_stop_device_discovery());
}

void BluetoothInstance::HandleCreateBonding(const picojson::value& msg) {
  callbacks_id_map_["CreateBonding"] = msg.get("reply_id").to_str();
  std::string address = msg.get("address").to_str();
  CAPI(bt_device_create_bond(address.c_str()));
}

void BluetoothInstance::HandleDestroyBonding(const picojson::value& msg) {
  callbacks_id_map_["DestroyBonding"] = msg.get("reply_id").to_str();
  std::string address = msg.get("address").to_str();
  CAPI(bt_device_destroy_bond(address.c_str()));
}

void BluetoothInstance::HandleRFCOMMListen(const picojson::value& msg) {
  int socket_fd = 0;
  int error = 0;

  picojson::value::object o;
  o["cmd"] = picojson::value("");
  o["reply_id"] = msg.get("reply_id");

  CAPI_ERR(
      bt_socket_create_rfcomm(msg.get("uuid").to_str().c_str(), &socket_fd),
      error);
  if (error != BT_ERROR_NONE) {
    o["error"] = picojson::value(static_cast<double>(2));
    if (error == BT_ERROR_INVALID_PARAMETER)
      o["error"] = picojson::value(static_cast<double>(1));
    InternalPostMessage(picojson::value(o));
    return;
  }

  socket_connected_map_[socket_fd] = false;

  CAPI_ERR(bt_socket_listen_and_accept_rfcomm(socket_fd, 0), error);
  if (error) {
    o["error"] = picojson::value(static_cast<double>(1));
    InternalPostMessage(picojson::value(o));
    return;
  }

  o["error"] = picojson::value(static_cast<double>(0));
  // give the listened socket to JS and store it in service_handler
  o["server_fd"] = picojson::value(static_cast<double>(socket_fd));
  InternalPostMessage(picojson::value(o));
}

void BluetoothInstance::HandleConnectToService(const picojson::value& msg) {
  int error = 0;

  CAPI_ERR(
      bt_socket_connect_rfcomm(msg.get("address").to_str().c_str(),
                               msg.get("uuid").to_str().c_str()),
      error);
  if (error != BT_ERROR_NONE) {
    picojson::value::object o;
    o["error"] = picojson::value(static_cast<double>(1));
    o["cmd"] = picojson::value("");
    o["reply_id"] = msg.get("reply_id");
    InternalPostMessage(picojson::value(o));
  } else {
    callbacks_id_map_["ConnectToService"] = msg.get("reply_id").to_str();
  }
}

void BluetoothInstance::HandleSocketWriteData(const picojson::value& msg) {
  picojson::value::object o;
  std::string data = msg.get("data").to_str();
  int socket = static_cast<int>(msg.get("socket_fd").get<double>());

  CAPI(bt_socket_send_data(socket, data.c_str(),
                           static_cast<int>(data.size())));
  o["size"] = picojson::value(static_cast<double>(data.size()));

  InternalSetSyncReply(picojson::value(o));
}

void BluetoothInstance::HandleCloseSocket(const picojson::value& msg) {
  picojson::value::object o;
  int error = 0;
  int socket = static_cast<int>(msg.get("socket_fd").get<double>());

  CAPI_ERR(bt_socket_disconnect_rfcomm(socket), error);
  o["error"] = picojson::value(error != BT_ERROR_NONE);
  o["cmd"] = picojson::value("");
  o["reply_id"] = msg.get("reply_id");
  o["capi"] = picojson::value(true);
  InternalPostMessage(picojson::value(o));
}

void BluetoothInstance::HandleUnregisterServer(const picojson::value& msg) {
  int socket = static_cast<int>(msg.get("server_fd").get<double>());
  int error = 0;

  CAPI_ERR(bt_socket_destroy_rfcomm(socket), error);

  // CAPI calls callback only if there is a socket connected
  if (socket_connected_map_[socket] == false) {
    picojson::value::object o;
    o["error"] = picojson::value(error != BT_ERROR_NONE);
    o["cmd"] = picojson::value("");
    o["reply_id"] = msg.get("reply_id");
    o["socket_fd"] = picojson::value(static_cast<double>(socket));
    InternalPostMessage(picojson::value(o));
  } else {
    callbacks_id_map_["RFCOMMsocketDestroy"] = msg.get("reply_id").to_str();
  }
}

void BluetoothInstance::HandleRegisterSinkApp(const picojson::value& msg) {
  picojson::value::object o;
  int error = 0;

  uint16_t data_type =
      static_cast<uint16_t>(msg.get("datatype").get<double>());

  char* app_id = NULL;
  CAPI_ERR(bt_hdp_register_sink_app(data_type, &app_id), error);
  o["error"] = picojson::value(error != BT_ERROR_NONE);
  o["app_id"] = picojson::value(app_id);
  o["cmd"] = picojson::value("");
  o["reply_id"] = msg.get("reply_id");
  InternalPostMessage(picojson::value(o));
}

void BluetoothInstance::HandleUnregisterSinkApp(const picojson::value& msg) {
  picojson::value::object o;
  int error = 0;

  CAPI_ERR(
      bt_hdp_unregister_sink_app(msg.get("app_id").to_str().c_str()),
      error);
  o["error"] = picojson::value(error != BT_ERROR_NONE);
  o["cmd"] = picojson::value("");
  o["reply_id"] = msg.get("reply_id");
  InternalPostMessage(picojson::value(o));
}

void BluetoothInstance::HandleConnectToSource(const picojson::value& msg) {
  picojson::value::object o;
  int error = 0;

  CAPI_ERR(
      bt_hdp_connect_to_source(msg.get("address").to_str().c_str(),
                               msg.get("app_id").to_str().c_str()),
      error);
  if (error != BT_ERROR_NONE) {
    o["error"] = picojson::value(static_cast<double>(2));
    if (error == BT_ERROR_INVALID_PARAMETER)
      o["error"] = picojson::value(static_cast<double>(1));
    o["cmd"] = picojson::value("");
    o["reply_id"] = msg.get("reply_id");
    InternalPostMessage(picojson::value(o));
  } else {
    callbacks_id_map_["ConnectToSource"] = msg.get("reply_id").to_str();
  }
}

void BluetoothInstance::HandleDisconnectSource(const picojson::value& msg) {
  picojson::value::object o;
  int error = 0;

  int channel = static_cast<int>(msg.get("channel").get<double>());

  CAPI_ERR(bt_hdp_disconnect(msg.get("address").to_str().c_str(), channel),
           error);
  if (error != BT_ERROR_NONE) {
    o["error"] = picojson::value(true);
    o["cmd"] = picojson::value("");
    o["reply_id"] = msg.get("reply_id");
    InternalPostMessage(picojson::value(o));
  } else {
    callbacks_id_map_["DisconnectSource"] = msg.get("reply_id").to_str();
  }
}

void BluetoothInstance::HandleSendHealthData(const picojson::value& msg) {
  picojson::value::object o;
  int error = 0;

  std::string data = msg.get("data").to_str();
  int channel = static_cast<int>(msg.get("channel").get<double>());

  CAPI_ERR(
      bt_hdp_send_data(channel, data.c_str(), static_cast<int>(data.size())),
      error);
  if (error != BT_ERROR_NONE) {
    o["error"] = picojson::value(true);
    o["cmd"] = picojson::value("");
    o["reply_id"] = msg.get("reply_id");
    InternalPostMessage(picojson::value(o));
  } else {
    callbacks_id_map_["SendHealthData"] = msg.get("reply_id").to_str();
  }
}

void BluetoothInstance::FlushPendingMessages() {
  // Flush previous pending messages.
  if (queue_.empty())
    return;

  MessageQueue::iterator it;
  for (it = queue_.begin(); it != queue_.end(); ++it)
    PostMessage((*it).serialize().c_str());
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

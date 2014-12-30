// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bluetooth/bluetooth_instance_capi.h"

#include "common/picojson.h"
#include "tizen/tizen.h"

namespace {

const char kEmptyStr[] = "";
const int kNoError = BT_ERROR_NONE;

const char kPowered[] = "Powered";
const char kName[] = "Name";
const char kVisible[] = "Discoverable";
const char kSetAdapterProperty[] = "SetAdapterProperty";
const char kDiscoverDevices[] = "DiscoverDevices";
const char kStopDiscovery[] = "StopDiscovery";
const char kCreateBonding[] = "CreateBonding";
const char kDestroyBonding[] = "DestroyBonding";
const char kRFCOMMListen[] = "RFCOMMListen";
const char kConnectToService[] = "ConnectToService";
const char kCloseSocket[] = "CloseSocket";
const char kUnregisterServer[] = "UnregisterServer";
const char kRegisterSinkApp[] = "RegisterSinkApp";
const char kUnregisterSinkApp[] = "UnregisterSinkApp";
const char kConnectToSource[] = "ConnectToSource";
const char kDisconnectSource[] = "DisconnectSource";
const char kSendHealthData[] = "SendHealthData";

const char kAdapterUpdated[] = "AdapterUpdated";
const char kDiscoveryFinished[] = "DiscoveryFinished";
const char kDeviceFound[] = "DeviceFound";
const char kDeviceRemoved[] = "DeviceRemoved";
const char kBondedDevice[] = "BondedDevice";
const char kRFCOMMSocketAccept[] = "RFCOMMSocketAccept";
const char kRFCOMMsocketDestroy[] = "RFCOMMsocketDestroy";
const char SocketHasData[] = "SocketHasData";

inline const char* BoolToString(bool b) {
  return b ? "true" : "false";
}

int CapiErrorToJs(int err) {
  return (err == BT_ERROR_NONE              ? NO_ERROR :
          err == BT_ERROR_INVALID_PARAMETER ? INVALID_VALUES_ERR :
                                              UNKNOWN_ERR);
}

}  // anonymous namespace

// Macros calling bluetooth C API and handling error cases
#define CAPI(fnc, msg)                                                         \
  do {                                                                         \
    int _er = (fnc);                                                           \
    if (_er != BT_ERROR_NONE) {                                                \
      LOG_ERR(#fnc " failed with error: " << _er);                             \
      PostAsyncError(msg.get("reply_id").to_str(), _er);                       \
      return;                                                                  \
    }                                                                          \
    StoreReplyId(msg);                                                         \
  } while (0)

// same CAPI macro for sync messages
#define CAPI_SYNC(fnc)                                                         \
  do {                                                                         \
    int _er = (fnc);                                                           \
    if (_er != BT_ERROR_NONE) {                                                \
      LOG_ERR(#fnc " failed with error: " << _er);                             \
      SendSyncError(_er);                                                      \
      return;                                                                  \
    }                                                                          \
  } while (0)

BluetoothInstance::BluetoothInstance() : are_bond_devices_known_(false) {}

void BluetoothInstance::Initialize() {
  // Initialize bluetooth CAPIs and register all needed callbacks.
  // By this way, bluetooth Web API will be updated according to BlueZ changes.
  bt_initialize();
  bt_adapter_set_state_changed_cb(OnStateChanged, this);
  bt_adapter_set_name_changed_cb(OnNameChanged, this);
  bt_adapter_set_visibility_mode_changed_cb(OnVisibilityChanged, this);
  bt_adapter_set_device_discovery_state_changed_cb(OnDiscoveryStateChanged,
      this);
  bt_device_set_bond_created_cb(OnBondCreated, this);
  bt_device_set_bond_destroyed_cb(OnBondDestroyed, this);
  bt_socket_set_connection_state_changed_cb(OnSocketConnected, this);
  bt_socket_set_data_received_cb(OnSocketHasData, this);
  bt_hdp_set_connection_state_changed_cb(OnHdpConnected, OnHdpDisconnected,
      this);
  bt_hdp_set_data_received_cb(OnHdpDataReceived, this);
}

BluetoothInstance::~BluetoothInstance() {
  bt_adapter_unset_state_changed_cb();
  bt_adapter_unset_name_changed_cb();
  bt_adapter_unset_visibility_mode_changed_cb();
  bt_adapter_unset_device_discovery_state_changed_cb();
  bt_device_unset_bond_created_cb();
  bt_device_unset_bond_destroyed_cb();
  bt_socket_unset_connection_state_changed_cb();
  bt_socket_unset_data_received_cb();
  bt_hdp_unset_connection_state_changed_cb();
  bt_hdp_unset_data_received_cb();
  bt_deinitialize();
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
  if (cmd == kDiscoverDevices)
    HandleDiscoverDevices(v);
  else if (cmd == kStopDiscovery)
    HandleStopDiscovery(v);
  else if (cmd == kSetAdapterProperty)
    HandleSetAdapterProperty(v);
  else if (cmd == kCreateBonding)
    HandleCreateBonding(v);
  else if (cmd == kDestroyBonding)
    HandleDestroyBonding(v);
  else if (cmd == kRFCOMMListen)
    HandleRFCOMMListen(v);
  else if (cmd == kConnectToService)
    HandleConnectToService(v);
  else if (cmd == kCloseSocket)
    HandleCloseSocket(v);
  else if (cmd == kUnregisterServer)
    HandleUnregisterServer(v);
  else if (cmd == kRegisterSinkApp)
    HandleRegisterSinkApp(v);
  else if (cmd == kUnregisterSinkApp)
    HandleUnregisterSinkApp(v);
  else if (cmd == kConnectToSource)
    HandleConnectToSource(v);
  else if (cmd == kDisconnectSource)
    HandleDisconnectSource(v);
  else if (cmd == kSendHealthData)
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

  // fill known_devices array on javascript side when bluetooth is enabled
  // the first time.
  if (!obj->are_bond_devices_known_)
    bt_adapter_foreach_bonded_device(OnKnownBondedDevice, obj);

  picojson::value::object o;
  o["Powered"] = picojson::value(
      BoolToString(adapter_state == BT_ADAPTER_ENABLED));

  obj->IsJsReplyId(kPowered) ? obj->PostAsyncReply(kPowered, result, o)
                             : obj->SendCmdToJs(kAdapterUpdated, o);
}

void BluetoothInstance::OnNameChanged(char* name, void* user_data) {
  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);

  picojson::value::object o;
  o["Name"] = picojson::value(name);

  obj->IsJsReplyId(kName) ? obj->PostAsyncReply(kName, kNoError, o)
                          : obj->SendCmdToJs(kAdapterUpdated, o);
}

void BluetoothInstance::OnVisibilityChanged(int result,
    bt_adapter_visibility_mode_e visibility_mode, void* user_data) {
  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);

  picojson::value::object o;
  o["Discoverable"] = picojson::value(BoolToString(
      visibility_mode == BT_ADAPTER_VISIBILITY_MODE_NON_DISCOVERABLE));

  obj->IsJsReplyId(kVisible) ? obj->PostAsyncReply(kVisible, result, o)
                             : obj->SendCmdToJs(kAdapterUpdated, o);
}

void BluetoothInstance::OnDiscoveryStateChanged(int result,
    bt_adapter_device_discovery_state_e discovery_state,
    bt_adapter_device_discovery_info_s* discovery_info, void* user_data) {
  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);

  switch (discovery_state) {
    case BT_ADAPTER_DEVICE_DISCOVERY_STARTED: {
      obj->PostAsyncReply(kDiscoverDevices, result);
      break;
    }
    case BT_ADAPTER_DEVICE_DISCOVERY_FINISHED: {
      if (obj->IsJsReplyId(kStopDiscovery)) {
        obj->PostAsyncReply(kStopDiscovery, result);
      } else {
        picojson::value::object o;
        obj->SendCmdToJs(kDiscoveryFinished, o);
      }
      break;
    }
    case BT_ADAPTER_DEVICE_DISCOVERY_FOUND: {
      picojson::value::object o;
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
        bt_adapter_get_bonded_device_info(discovery_info->remote_address,
                                          &device_info);

        if (!device_info) {
          LOG_ERR("device_info is null");
          break;
        }

        if (!device_info->is_bonded) {
          LOG_ERR("remote device should be bonded!");
          break;
        }

        paired = true;
        trusted = device_info->is_authorized;
        connected = device_info->is_connected;
        bt_adapter_free_device_info(device_info);
      }

      o["Paired"] = picojson::value(BoolToString(paired));
      o["Trusted"] = picojson::value(BoolToString(trusted));
      o["Connected"] = picojson::value(BoolToString(connected));
      o["found_on_discovery"] = picojson::value(true);
      obj->SendCmdToJs(kDeviceFound, o);
      break;
    }
    case BT_ADAPTER_DEVICE_DISCOVERY_REMOVED: {
      picojson::value::object o;
      o["Address"] = picojson::value(discovery_info->remote_address);
      obj->SendCmdToJs(kDeviceRemoved, o);
      break;
    }
    default:
      LOG_ERR("Unknown discovery state callback!");
      break;
  }
}

bool BluetoothInstance::OnKnownBondedDevice(bt_device_info_s* device_info,
    void* user_data) {
  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);

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
  obj->SendCmdToJs(kBondedDevice, o);
  obj->are_bond_devices_known_ = true;
  return true;
}

void BluetoothInstance::OnBondCreated(int result, bt_device_info_s* device_info,
    void* user_data) {
  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);

  if (!device_info) {
    LOG_ERR("device_info is NULL!");
    return;
  }

  picojson::value::object o;
  o["capi"] = picojson::value(true);
  obj->PostAsyncReply(kCreateBonding, result, o);
}

void BluetoothInstance::OnBondDestroyed(int result, char* remote_address,
    void* user_data) {
  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);

  if (!remote_address)
    LOG_ERR("remote_address is NULL!");

  picojson::value::object o;
  o["capi"] = picojson::value(true);
  obj->PostAsyncReply(kDestroyBonding, result, o);
}

void BluetoothInstance::OnSocketConnected(int result,
    bt_socket_connection_state_e connection_state,
    bt_socket_connection_s* connection,
    void* user_data) {
  if (!connection) {
    LOG_ERR("connection is NULL!");
    return;
  }

  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);

  if (connection_state == BT_SOCKET_CONNECTED &&
      connection->local_role == BT_SOCKET_SERVER) {
    picojson::value::object o;
    o["uuid"] = picojson::value(connection->service_uuid);
    o["socket_fd"] =
        picojson::value(static_cast<double>(connection->socket_fd));
    o["peer"] = picojson::value(connection->remote_address);
    obj->socket_connected_map_[connection->socket_fd] = true;
    obj->SendCmdToJs(kRFCOMMSocketAccept, o);

  } else if (connection_state == BT_SOCKET_CONNECTED &&
             connection->local_role == BT_SOCKET_CLIENT) {
    picojson::value::object o;
    o["uuid"] = picojson::value(connection->service_uuid);
    o["socket_fd"] =
        picojson::value(static_cast<double>(connection->socket_fd));
    o["peer"] = picojson::value(connection->remote_address);
    obj->socket_connected_map_[connection->socket_fd] = true;
    obj->PostAsyncReply(kConnectToService, result, o);

  } else if (connection_state == BT_SOCKET_DISCONNECTED) {
    picojson::value::object o;
    o["socket_fd"] =
        picojson::value(static_cast<double>(connection->socket_fd));
    obj->socket_connected_map_[connection->socket_fd] = false;
    obj->PostAsyncReply(kRFCOMMsocketDestroy, result, o);
  } else {
    LOG_ERR("Unknown role!");
  }
}

void BluetoothInstance::OnSocketHasData(bt_socket_received_data_s* data,
    void* user_data) {
  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);

  if (!data) {
    LOG_ERR("data is NULL");
    return;
  }

  picojson::value::object o;
  o["socket_fd"] = picojson::value(static_cast<double>(data->socket_fd));
  o["data"] = picojson::value(static_cast<std::string>(data->data));
  obj->SendCmdToJs("SocketHasData", o);
}

void BluetoothInstance::OnHdpConnected(int result, const char* remote_address,
    const char* app_id, bt_hdp_channel_type_e type, unsigned int channel,
    void* user_data) {
  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);

  picojson::value::object o;
  o["address"] = picojson::value(remote_address);
  o["app_id"] = picojson::value(app_id);
  o["channel_type"] = picojson::value(static_cast<double>(type));
  o["channel"] = picojson::value(static_cast<double>(channel));
  o["connected"] = picojson::value("true");
  obj->PostAsyncReply(kConnectToSource, result, o);
}

void BluetoothInstance::OnHdpDisconnected(int result,
    const char* remote_address, unsigned int channel, void* user_data) {
  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);

  picojson::value::object o;
  o["address"] = picojson::value(remote_address);
  o["channel"] = picojson::value(static_cast<double>(channel));
  o["connected"] = picojson::value("false");
  obj->PostAsyncReply(kDisconnectSource, result, o);
}

void BluetoothInstance::OnHdpDataReceived(unsigned int channel,
    const char* data, unsigned int size, void* user_data) {
  BluetoothInstance* obj = static_cast<BluetoothInstance*>(user_data);

  picojson::value::object o;
  o["channel"] = picojson::value(static_cast<double>(channel));
  o["data"] = picojson::value(data);
  o["size"] = picojson::value(static_cast<double>(size));
  obj->PostAsyncReply(kSendHealthData, kNoError, o);
}

void BluetoothInstance::HandleGetDefaultAdapter(const picojson::value& msg) {
  char* name = NULL;
  CAPI_SYNC(bt_adapter_get_name(&name));

  char* address = NULL;
  CAPI_SYNC(bt_adapter_get_address(&address));

  bt_adapter_state_e state = BT_ADAPTER_DISABLED;
  CAPI_SYNC(bt_adapter_get_state(&state));

  bool powered = false;
  bool visible = false;

  if (state == BT_ADAPTER_ENABLED) {
    powered = true;
    bt_adapter_visibility_mode_e mode =
        BT_ADAPTER_VISIBILITY_MODE_NON_DISCOVERABLE;

    CAPI_SYNC(bt_adapter_get_visibility(&mode, NULL));
    visible = (mode > 0);
  }

  picojson::value::object o;
  o["name"] = picojson::value(name);
  o["address"] = picojson::value(address);
  o["powered"] = picojson::value(powered);
  o["visible"] = picojson::value(visible);
  picojson::value v(o);
  SendSyncReply(v.serialize().c_str());

  // fill known_devices array on javascript side if bluetooth is enabled.
  // else we do it when receiving bluetooth enabled event the first time.
  if (powered)
    bt_adapter_foreach_bonded_device(OnKnownBondedDevice, this);
}

void BluetoothInstance::HandleSetAdapterProperty(const picojson::value& msg) {
  std::string property = msg.get("property").to_str();
  if (property == kPowered) {
    if (msg.get("value").get<bool>())
      CAPI(bt_adapter_enable(), msg);
    else
      CAPI(bt_adapter_disable(), msg);

  } else if (property == kName) {
    CAPI(bt_adapter_set_name(msg.get("value").to_str().c_str()), msg);

  } else if (property == kVisible) {
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
    CAPI(bt_adapter_set_visibility(discoverable_mode, timeout), msg);
  }
  // All adapter properties use the same json cmd, so in this case the js
  // reply_id is paired with the adapter property name.
  reply_id_map_[property] = reply_id_map_[kSetAdapterProperty];
  RemoveReplyId(kSetAdapterProperty);
}

void BluetoothInstance::HandleDiscoverDevices(const picojson::value& msg) {
  CAPI(bt_adapter_start_device_discovery(), msg);
}

void BluetoothInstance::HandleStopDiscovery(const picojson::value& msg) {
  bool is_discovering = false;
  bt_adapter_is_discovering(&is_discovering);
  if (!is_discovering) {
    PostResult(kEmptyStr, msg.get("reply_id").to_str(), kNoError);
    return;
  }
  CAPI(bt_adapter_stop_device_discovery(), msg);
}

void BluetoothInstance::HandleCreateBonding(const picojson::value& msg) {
  CAPI(bt_device_create_bond(msg.get("address").to_str().c_str()), msg);
}

void BluetoothInstance::HandleDestroyBonding(const picojson::value& msg) {
  CAPI(bt_device_destroy_bond(msg.get("address").to_str().c_str()), msg);
}

void BluetoothInstance::HandleRFCOMMListen(const picojson::value& msg) {
  int socket_fd = 0;
  CAPI(bt_socket_create_rfcomm(msg.get("uuid").to_str().c_str(), &socket_fd),
      msg);

  socket_connected_map_[socket_fd] = false;

  CAPI(bt_socket_listen_and_accept_rfcomm(socket_fd, 0), msg);

  picojson::value::object o;
  // give the listened socket to JS and store it in service_handler
  o["server_fd"] = picojson::value(static_cast<double>(socket_fd));
  PostAsyncReply(kRFCOMMListen, kNoError, o);
}

void BluetoothInstance::HandleConnectToService(const picojson::value& msg) {
  CAPI(bt_socket_connect_rfcomm(msg.get("address").to_str().c_str(),
      msg.get("uuid").to_str().c_str()), msg);
}

void BluetoothInstance::HandleSocketWriteData(const picojson::value& msg) {
  std::string data = msg.get("data").to_str();
  int socket = static_cast<int>(msg.get("socket_fd").get<double>());

  CAPI_SYNC(bt_socket_send_data(socket, data.c_str(),
                                static_cast<int>(data.size())));

  picojson::value::object o;
  o["size"] = picojson::value(static_cast<double>(data.size()));
  picojson::value v(o);
  SendSyncReply(v.serialize().c_str());
}

void BluetoothInstance::HandleCloseSocket(const picojson::value& msg) {
  int socket = static_cast<int>(msg.get("socket_fd").get<double>());
  CAPI(bt_socket_disconnect_rfcomm(socket), msg);
  picojson::value::object o;
  o["capi"] = picojson::value(true);
  PostAsyncReply(kCloseSocket, kNoError, o);
}

void BluetoothInstance::HandleUnregisterServer(const picojson::value& msg) {
  int socket = static_cast<int>(msg.get("server_fd").get<double>());
  CAPI(bt_socket_destroy_rfcomm(socket), msg);
  // if socket is not connected, OnSocketConnected() cb won't be triggered.
  // In that case, we directly send a success post message to JavaScript.
  if (socket_connected_map_[socket] == false) {
    picojson::value::object o;
    o["socket_fd"] = picojson::value(static_cast<double>(socket));
    PostAsyncReply(kUnregisterServer, kNoError, o);
  }
}

void BluetoothInstance::HandleRegisterSinkApp(const picojson::value& msg) {
  uint16_t data_type =
      static_cast<uint16_t>(msg.get("datatype").get<double>());

  char* app_id = NULL;
  CAPI(bt_hdp_register_sink_app(data_type, &app_id), msg);
  picojson::value::object o;
  o["app_id"] = picojson::value(app_id);
  PostAsyncReply(kRegisterSinkApp, kNoError, o);
}

void BluetoothInstance::HandleUnregisterSinkApp(const picojson::value& msg) {
  CAPI(bt_hdp_unregister_sink_app(msg.get("app_id").to_str().c_str()), msg);
  PostAsyncReply(kUnregisterSinkApp, kNoError);
}

void BluetoothInstance::HandleConnectToSource(const picojson::value& msg) {
  CAPI(bt_hdp_connect_to_source(msg.get("address").to_str().c_str(),
      msg.get("app_id").to_str().c_str()), msg);
}

void BluetoothInstance::HandleDisconnectSource(const picojson::value& msg) {
  int channel = static_cast<int>(msg.get("channel").get<double>());
  CAPI(bt_hdp_disconnect(msg.get("address").to_str().c_str(), channel), msg);
}

void BluetoothInstance::HandleSendHealthData(const picojson::value& msg) {
  std::string data = msg.get("data").to_str();
  int channel = static_cast<int>(msg.get("channel").get<double>());
  CAPI(bt_hdp_send_data(channel, data.c_str(), static_cast<int>(data.size())),
       msg);
}

bool BluetoothInstance::IsJsReplyId(const std::string& cmd) {
  return !reply_id_map_[cmd].empty();
}

void BluetoothInstance::StoreReplyId(const picojson::value& msg) {
  reply_id_map_[msg.get("cmd").to_str()] = msg.get("reply_id").to_str();
}

void BluetoothInstance::RemoveReplyId(const std::string& cmd) {
  if (IsJsReplyId(cmd))
    reply_id_map_.erase(cmd);
}

void BluetoothInstance::PostAsyncError(const std::string& reply_id, int error) {
  PostResult(kEmptyStr, reply_id, error);
}

void BluetoothInstance::PostAsyncReply(const std::string& cmd, int error) {
  PostResult(kEmptyStr, reply_id_map_[cmd], error);
  RemoveReplyId(cmd);
}

void BluetoothInstance::PostAsyncReply(const std::string& cmd, int error,
    picojson::value::object& o) {
  PostResult(kEmptyStr, reply_id_map_[cmd], error, o);
  RemoveReplyId(cmd);
}

void BluetoothInstance::SendCmdToJs(const std::string& cmd,
    picojson::value::object& o) {
  PostResult(cmd, kEmptyStr, kNoError, o);
}

void BluetoothInstance::SendSyncError(int error) {
  picojson::value::object o;
  o["error"] = picojson::value(static_cast<double>(CapiErrorToJs(error)));
  picojson::value v(o);
  SendSyncReply(v.serialize().c_str());
}

void BluetoothInstance::PostResult(const std::string& cmd,
    const std::string& reply_id, int error) {
  picojson::value::object o;
  PostResult(cmd, reply_id, error, o);
}

void BluetoothInstance::PostResult(const std::string& cmd,
    const std::string& reply_id, int error, picojson::value::object& o) {
  o["cmd"] = picojson::value(cmd);
  o["reply_id"] = picojson::value(reply_id);
  o["error"] = picojson::value(static_cast<double>(CapiErrorToJs(error)));
  picojson::value v(o);
  PostMessage(v.serialize().c_str());
}

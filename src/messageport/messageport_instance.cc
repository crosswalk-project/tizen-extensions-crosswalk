// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "messageport/messageport_instance.h"

#include <message-port.h>
#include <string.h>

#include <map>
#include <string>
#include <vector>

MessageportInstance::MessageportIdToInstanceMap
      MessageportInstance::mp_id_to_instance_map_;

namespace {

bool ErrorIfMessageHasNoKey(const picojson::value& msg,
      const std::string& key, picojson::value::object& reply) {
  if (!msg.contains(key)) {
    std::cerr << "Required parameter \"" << key << "\" missing from message\n";

    reply["invalid_parameter"] = picojson::value(true);
    return true;
  }

  return false;
}

};  // namespace

void MessageportInstance::RegisterLocalMessageport(int mp_id,
      MessageportInstance *instance) {
  MessageportInstance::mp_id_to_instance_map_[mp_id] = instance;
}

MessageportInstance* MessageportInstance::GetInstanceByPortId(int mp_id) {
  MessageportIdToInstanceMap::iterator it =
        MessageportInstance::mp_id_to_instance_map_.find(mp_id);

  if (it == mp_id_to_instance_map_.end())
    return 0;
  return it->second;
}

void MessageportInstance::BundleJsonIterator(
      const char *k, const char *v, void *d) {
  picojson::value::array *array = static_cast<picojson::value::array *>(d);
  picojson::value::object o;
  o["key"] = picojson::value(k);
  o["value"] = picojson::value(v);
  array->push_back(picojson::value(o));
}

void MessageportInstance::OnReceiveLocalMessage(
      int id, const char* remote_app_id, const char* remote_port,
      bool trusted_message, bundle* data) {
  picojson::value::object o;

  o["cmd"] = picojson::value("LocalMessageReceived");
  o["id"] = picojson::value(static_cast<double>(id));
  o["remoteAppId"] = picojson::value(remote_app_id);
  o["remotePort"] = picojson::value(remote_port);
  o["trusted"] = picojson::value(trusted_message);

  picojson::value::array d;
  bundle_iterate(data, BundleJsonIterator, &d);

  o["data"] = picojson::value(d);

  PostMessage(picojson::value(o).serialize().c_str());
}

void MessageportInstance::OnReceiveLocalMessageThunk(
      int id, const char* remote_app_id, const char* remote_port,
      bool trusted_message, bundle* data) {
  MessageportInstance* self = MessageportInstance::GetInstanceByPortId(id);
  if (!self) {
    std::cerr << "Could not find Messageport by id: " << id << "\n";
    return;
  }

  self->OnReceiveLocalMessage(id, remote_app_id, remote_port, trusted_message,
        data);
}

void MessageportInstance::HandleSyncMessage(const char *message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cerr << "Ignoring unparsable sync message: " << message << "\n";
    std::cerr << "Error was: " << err << "\n";
    return;
  }

  picojson::value::object o;
  if (!ErrorIfMessageHasNoKey(v, "cmd", o)) {
    std::string cmd = v.get("cmd").to_str();

    if (cmd == "RequestLocalMessagePort") {
      HandleRequestLocalMessagePort(v, o);
    } else if (cmd == "RequestRemoteMessagePort") {
      HandleRequestRemoteMessagePort(v, o);
    } else if (cmd == "SendMessage") {
      HandleSendMessage(v, o);
    } else {
      std::cerr << "Ignoring unknown command: " << cmd << "\n";
      return;
    }
  } else {
    std::cerr << "Message has no command, ignoring\n";
    return;
  }

  SendSyncReply(picojson::value(o).serialize().c_str());
}

void MessageportInstance::HandleRequestLocalMessagePort(
        const picojson::value& msg, picojson::value::object& o) {
  if (ErrorIfMessageHasNoKey(msg, "messagePortName", o))
    return;
  if (ErrorIfMessageHasNoKey(msg, "trusted", o))
    return;

  std::string message_port_name = msg.get("messagePortName").to_str();
  int mp_id;

  if (msg.get("trusted").get<bool>()) {
    mp_id = messageport_register_trusted_local_port(message_port_name.c_str(),
          OnReceiveLocalMessageThunk);
  } else {
    mp_id = messageport_register_local_port(message_port_name.c_str(),
          OnReceiveLocalMessageThunk);
  }

  if (mp_id < 0) {
    switch (mp_id) {
      case MESSAGEPORT_ERROR_INVALID_PARAMETER:
        o["invalid_parameter"] = picojson::value(true);
        break;
      case MESSAGEPORT_ERROR_OUT_OF_MEMORY:
        o["out_of_memory"] = picojson::value(true);
        break;
      case MESSAGEPORT_ERROR_IO_ERROR:
        o["io_error"] = picojson::value(true);
        break;
      default:
        o["unknown_error"] = picojson::value(true);
    }
  } else {
    MessageportInstance::RegisterLocalMessageport(mp_id, this);
    o["id"] = picojson::value(static_cast<double>(mp_id));
    o["success"] = picojson::value(true);
  }
}

void MessageportInstance::HandleRequestRemoteMessagePort(
      const picojson::value& msg, picojson::value::object& o) {
  if (ErrorIfMessageHasNoKey(msg, "messagePortName", o))
    return;
  if (ErrorIfMessageHasNoKey(msg, "trusted", o))
    return;
  if (ErrorIfMessageHasNoKey(msg, "appId", o))
    return;

  std::string message_port_name = msg.get("messagePortName").to_str();
  std::string app_id = msg.get("appId").to_str();
  int ret_val;
  bool exist;

  if (msg.get("trusted").get<bool>()) {
    ret_val = messageport_check_trusted_remote_port(app_id.c_str(),
          message_port_name.c_str(), &exist);
  } else {
    ret_val = messageport_check_remote_port(app_id.c_str(),
          message_port_name.c_str(), &exist);
  }

  if (!exist) {
    o["not_found"] = picojson::value(true);
    return;
  }

  if (ret_val < 0) {
    switch (ret_val) {
      case MESSAGEPORT_ERROR_INVALID_PARAMETER:
        o["invalid_parameter"] = picojson::value(true);
        break;
      case MESSAGEPORT_ERROR_OUT_OF_MEMORY:
        o["out_of_memory"] = picojson::value(true);
        break;
      case MESSAGEPORT_ERROR_IO_ERROR:
        o["io_error"] = picojson::value(true);
        break;
      case MESSAGEPORT_ERROR_CERTIFICATE_NOT_MATCH:
        o["certificate_error"] = picojson::value(true);
        break;
      default:
        o["unknown_error"] = picojson::value(true);
    }
  } else {
    o["success"] = picojson::value(true);
  }
}

void MessageportInstance::HandleSendMessage(
      const picojson::value& msg, picojson::value::object& o) {
  if (ErrorIfMessageHasNoKey(msg, "messagePortName", o))
    return;
  if (ErrorIfMessageHasNoKey(msg, "trusted", o))
    return;
  if (ErrorIfMessageHasNoKey(msg, "appId", o))
    return;
  if (ErrorIfMessageHasNoKey(msg, "localPort", o))
    return;
  if (ErrorIfMessageHasNoKey(msg, "data", o))
    return;

  std::string app_id = msg.get("appId").to_str();
  std::string message_port_name = msg.get("messagePortName").to_str();
  int local_port = static_cast<int>(msg.get("localPort").get<double>());
  std::vector<picojson::value> data = msg.get("data").get<picojson::array>();
  int ret_val;
  bundle* bundle = bundle_create();

  for (picojson::value::array::iterator it = data.begin();
          it != data.end(); it++) {
    bundle_add(bundle, (*it).get("key").to_str().c_str(),
          (*it).get("value").to_str().c_str());
  }

  if (msg.get("trusted").get<bool>()) {
    if (local_port < 0) {
      ret_val = messageport_send_trusted_message(app_id.c_str(),
            message_port_name.c_str(), bundle);
    } else {
      ret_val = messageport_send_bidirectional_trusted_message(local_port,
            app_id.c_str(), message_port_name.c_str(), bundle);
    }
  } else {
    if (local_port < 0) {
      ret_val = messageport_send_message(app_id.c_str(),
            message_port_name.c_str(), bundle);
    } else {
      ret_val = messageport_send_bidirectional_message(local_port,
            app_id.c_str(), message_port_name.c_str(), bundle);
    }
  }

  bundle_free(bundle);

  if (ret_val < 0) {
    switch (ret_val) {
      case MESSAGEPORT_ERROR_INVALID_PARAMETER:
        o["invalid_parameter"] = picojson::value(true);
        return;
      case MESSAGEPORT_ERROR_OUT_OF_MEMORY:
        o["out_of_memory"] = picojson::value(true);
        return;
      case MESSAGEPORT_ERROR_MESSAGEPORT_NOT_FOUND:
        o["messageport_not_found"] = picojson::value(true);
        return;
      case MESSAGEPORT_ERROR_CERTIFICATE_NOT_MATCH:
        o["certificate_not_found"] = picojson::value(true);
        return;
      case MESSAGEPORT_ERROR_MAX_EXCEEDED:
        o["max_exceeded"] = picojson::value(true);
        return;
      case MESSAGEPORT_ERROR_IO_ERROR:
        o["io_error"] = picojson::value(true);
        return;
      default:
        o["unknown_error"] = picojson::value(true);
    }
  } else {
    o["success"] = picojson::value(true);
  }
}

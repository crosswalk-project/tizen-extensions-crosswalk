// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "phone/phone_instance.h"

#include <gio/gio.h>
#include <thread>
#include <string>

#define TIZEN_PREFIX            "org.tizen"
#define PHONE_SERVICE           TIZEN_PREFIX ".phone"
#define PHONE_IFACE             TIZEN_PREFIX ".Phone"
#define PHONE_OBJ_PATH          "/"

static void* run_loop() {
  GMainLoop* main_loop = NULL;
  main_loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run(main_loop);
}

PhoneInstance::PhoneInstance() {
  std::cout << "Creating phone instance" << "\n";
  if (t_ == NULL)
    t_= new std::thread(run_loop);
}

PhoneInstance::~PhoneInstance() {
  std::cout << "Deleting phone instance" << "\n";
}

void PhoneInstance::HandleMessage(const char* msg) {
  picojson::value v;

  std::string err;
  picojson::parse(v, msg, msg + strlen(msg), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "SelectRemoteDevice") {
    HandleSelectRemoteDevice(v);
  } else if (cmd == "UnselectRemoteDevice") {
    HandleUnselectRemoteDevice(v);
  } else if (cmd == "GetSelectedRemoteDevice") {
    HandleGetSelectedRemoteDevice(v);
  } else if (cmd == "InvokeCall") {
    HandleInvokeCall(v);
  } else if (cmd == "AnswerCall") {
    HandleAnswerCall(v);
  } else if (cmd == "HangupCall") {
    HandleHangupCall(v);
  } else if (cmd == "ActiveCall") {
    HandleActiveCall(v);
  } else if (cmd == "MuteCall") {
    HandleMuteCall(v);
  } else if (cmd == "GetContacts") {
    HandleGetContacts(v);
  } else if (cmd == "GetCallHistory") {
    HandleGetCallHistory(v);
  } else if (cmd == "AddRemoteDeviceSelectedListener") {
    HandleAddRemoteDeviceSelectedListener(v);
  } else if (cmd == "RemoveRemoteDeviceSelectedListener") {
    HandleRemoveRemoteDeviceSelectedListener(v);
  } else if (cmd == "AddCallChangedListener") {
    HandleAddCallChangedListener(v);
  } else if (cmd == "RemoveCallChangedListener") {
    HandleRemoveCallChangedListener(v);
  } else if (cmd == "AddCallHistoryEntryAddedListener") {
    HandleAddCallHistoryEntryAddedListener(v);
  } else if (cmd == "RemoveCallHistoryEntryAddedListener") {
    HandleRemoveCallHistoryEntryAddedListener(v);
  } else if (cmd == "AddCallHistoryChangedListener") {
    HandleAddCallHistoryChangedListener(v);
  } else if (cmd == "RemoveCallHistoryChangedListener") {
    HandleRemoveCallHistoryChangedListener(v);
  } else {
    std::cout << "Ignoring unknown command: " << cmd << "\n";
  }
}

void PhoneInstance::HandleSyncMessage(const char* msg) {
  picojson::value v;
  std::string err;

  picojson::parse(v, msg, msg + strlen(msg), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "ActiveCall") {
    HandleActiveCall(v);
  } else {
    std::cout << "ASSERT NOT REACHED.\n";
  }
}

void PhoneInstance::SendSyncErrorReply(WebApiAPIErrors errorCode) {
  picojson::value::object o;
  o["isError"] = picojson::value(true);
  o["errorCode"] = picojson::value(static_cast<double>(errorCode));
  picojson::value v(o);
  SendSyncReply(v.serialize().c_str());
}

void PhoneInstance::SendSyncErrorReply(WebApiAPIErrors errorCode,
                                       const std::string& error_msg) {
  picojson::value::object o;
  o["isError"] = picojson::value(true);
  o["errorCode"] = picojson::value(static_cast<double>(errorCode));
  o["errorMessage"] = picojson::value(error_msg.c_str());
  picojson::value v(o);
  SendSyncReply(v.serialize().c_str());
}

void PhoneInstance::SendSyncSuccessReply() {
  picojson::value::object o;
  o["isError"] = picojson::value(false);
  picojson::value v(o);
  SendSyncReply(v.serialize().c_str());
}

void PhoneInstance::SendSyncSuccessReply(const picojson::value value) {
  picojson::value::object o;
  o["isError"] = picojson::value(false);
  o["value"] = value;
  picojson::value v(o);
  SendSyncReply(v.serialize().c_str());
}

void PhoneInstance::PostAsyncReply(const picojson::value& msg,
  picojson::value::object& reply) {
  reply["replyId"] = picojson::value(msg.get("replyId").get<double>());

  picojson::value v(reply);
  PostMessage(v.serialize().c_str());
}

void PhoneInstance::PostAsyncErrorReply(const picojson::value& msg,
    WebApiAPIErrors error_code) {
  picojson::value::object reply;
  reply["isError"] = picojson::value(true);
  reply["errorCode"] = picojson::value(static_cast<double>(error_code));
  PostAsyncReply(msg, reply);
}

void PhoneInstance::PostAsyncErrorReply(const picojson::value& msg,
    WebApiAPIErrors error_code, const std::string& error_msg) {
  picojson::value::object reply;
  reply["isError"] = picojson::value(true);
  reply["errorCode"] = picojson::value(static_cast<double>(error_code));
  reply["errorMessage"] = picojson::value(error_msg.c_str());
  PostAsyncReply(msg, reply);
}

void PhoneInstance::PostAsyncSuccessReply(const picojson::value& msg,
    picojson::value& value) {
  picojson::value::object reply;
  reply["isError"] = picojson::value(false);
  reply["value"] = value;
  PostAsyncReply(msg, reply);
}

void PhoneInstance::PostAsyncSuccessReply(const picojson::value& msg) {
  picojson::value::object reply;
  reply["isError"] = picojson::value(false);
  PostAsyncReply(msg, reply);
}

void PhoneInstance::handleSignal( GDBusConnection *connection,
                                  const gchar *sender_name,
                                  const gchar *object_path,
                                  const gchar *interface_name,
                                  const gchar *signal_name,
                                  GVariant *parameters,
                                  gpointer user_data)
{
  std::cout << "handleSignal entered\n";
  std::cout << "\tsender_name: " << sender_name << "\n";
  std::cout << "\tobject_path: " << object_path << "\n";
  std::cout << "\tinterface_name: " << interface_name << "\n";
  std::cout << "\tsignal_name: " << signal_name << "\n";

  PhoneInstance* instance = static_cast<PhoneInstance*>(user_data);
  if (!instance) {
    std::cout << "Failed to cast to instance..." << "\n";
    return;
  }

  if (!strcmp(signal_name, "RemoteDeviceSelected")) {
    const char *result;
    g_variant_get(parameters, "(s)", &result);
    if (result) {
      picojson::value value;
      std::string err;
      picojson::parse(value, result, result + strlen(result), &err);
      if (!err.empty()) {
        std::cout << "cannot parse result.\n";
        return;
      }
      picojson::value::object o;
      o["cmd"] = picojson::value("signal");
      o["signal_name"] = picojson::value(signal_name);
      o["signal_value"] = value;
      picojson::value msg(o);
      instance->PostMessage(msg.serialize().c_str());
    }
  } else if (!strcmp(signal_name, "CallChanged")) {
    GVariantIter *iter;
    g_variant_get(parameters, "(a{sv})", &iter);
    const char *key = NULL, *state = NULL, *line_id = NULL, *contact = NULL;
    GVariant *value;
    while (g_variant_iter_next(iter, "{sv}", &key, &value)) {
      if (!strcmp(key, "state")) {
        state = g_variant_get_string(value, NULL);
      }
      else if (!strcmp(key, "line_id")) {
        line_id = g_variant_get_string(value, NULL);
      }
      else if (!strcmp(key, "contact")) {
        contact = g_variant_get_string(value, NULL);
      }
    }
    picojson::value::object v;
    v["state"] = state ? picojson::value(state) : picojson::value("");
    v["line_id"] = line_id ? picojson::value(line_id) : picojson::value("");
    v["contact"] = contact ? picojson::value(contact) : picojson::value("");
    picojson::value::object o;
    o["cmd"] = picojson::value("signal");
    o["signal_name"] = picojson::value(signal_name);
    o["signal_value"] = picojson::value(v);
    picojson::value msg(o);
    instance->PostMessage(msg.serialize().c_str());
  } else if (!strcmp(signal_name, "CallHistoryEntryAdded")) {
    const char *result;
    g_variant_get(parameters, "(s)", &result);
    if (result) {
      picojson::value value;
      std::string err;
      picojson::parse(value, result, result + strlen(result), &err);
      if (!err.empty()) {
        std::cout << "cannot parse result.\n";
        return;
      }
      picojson::value::object o;
      o["cmd"] = picojson::value("signal");
      o["signal_name"] = picojson::value(signal_name);
      o["signal_value"] = value;
      picojson::value msg(o);
      instance->PostMessage(msg.serialize().c_str());
    }
  } else if (!strcmp(signal_name, "CallHistoryChanged") ||
             !strcmp(signal_name, "ContactsChanged")) {
    picojson::value::object o;
    o["cmd"] = picojson::value("signal");
    o["signal_name"] = picojson::value(signal_name);
    picojson::value msg(o);
    instance->PostMessage(msg.serialize().c_str());
  }
}

void PhoneInstance::HandleSelectRemoteDevice(
  const picojson::value& msg) {
  std::cout << "HandleSelectRemoteDevice entered.\n";

  if (!msg.contains("address")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  std::string btAddress = msg.get("address").to_str();
  picojson::value::object o;

  GError *error = NULL;
  g_dbus_connection_call_sync( g_bus_get_sync(G_BUS_TYPE_SESSION, NULL,NULL),
                               PHONE_SERVICE,
                               PHONE_OBJ_PATH,
                               PHONE_IFACE,
                               "SelectRemoteDevice",
                               g_variant_new("(s)", btAddress.c_str()),
                               NULL,
                               G_DBUS_CALL_FLAGS_NONE,
                               -1,
                               NULL,
                               &error);

  if (error) {
    // call the error callback to notify client about the error condition
    std::cerr << "Error occured '" << error->message << "'\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR, std::string(error->message));
    return;
  }

  PostAsyncSuccessReply(msg);
}

void PhoneInstance::HandleUnselectRemoteDevice(
  const picojson::value& msg) {
  std::cout << "HandleUnselectRemoteDevice entered.\n";
  picojson::value::object o;

  GError *error = NULL;
  g_dbus_connection_call_sync( g_bus_get_sync(G_BUS_TYPE_SESSION, NULL,NULL),
                               PHONE_SERVICE,
                               PHONE_OBJ_PATH,
                               PHONE_IFACE,
                               "UnselectRemoteDevice",
                               NULL,
                               NULL,
                               G_DBUS_CALL_FLAGS_NONE,
                               -1,
                               NULL,
                               &error);
  if (error) {
    // call the error callback to notify client about the error condition
    std::cerr << "Error occured '" << error->message << "'\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR, std::string(error->message));
    return;
  }

  PostAsyncSuccessReply(msg);
}

void PhoneInstance::HandleGetSelectedRemoteDevice(
  const picojson::value& msg) {
  std::cout << "GetSelectedRemoteDevice entered.\n";
  picojson::value::object o;

  GError *error = NULL;
  GVariant *reply;
  reply = g_dbus_connection_call_sync( g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL),
                                       PHONE_SERVICE,
                                       PHONE_OBJ_PATH,
                                       PHONE_IFACE,
                                       "GetSelectedRemoteDevice",
                                       NULL,
                                       NULL,
                                       G_DBUS_CALL_FLAGS_NONE,
                                       -1,
                                       NULL,
                                       &error);

  if (error) {
    // call the error callback to notify client about the error condition
    std::cerr << "Error occured '" << error->message << "'\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR, std::string(error->message));
  } else if (!reply) {
    std::cerr << "No reply\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR);
  } else {
    const char *str;
    g_variant_get(reply, "(s)", &str);
    if (str) {
      picojson::value value(str);
      PostAsyncSuccessReply(msg, value);
    } else {
      PostAsyncErrorReply(msg, UNKNOWN_ERR);
    }
  }

  if (reply)
    g_variant_unref(reply);
}

void PhoneInstance::HandleInvokeCall(const picojson::value& msg) {
  std::cout << "HandleInvokeCall entered.\n";

  if (!msg.contains("phoneNumber")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  std::string phoneNumber = msg.get("phoneNumber").to_str();
  picojson::value::object o;
  GError *error = NULL;
  g_dbus_connection_call_sync( g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL),
                               PHONE_SERVICE,
                               PHONE_OBJ_PATH,
                               PHONE_IFACE,
                               "Dial",
                               g_variant_new("(s)", phoneNumber.c_str()),
                               NULL,
                               G_DBUS_CALL_FLAGS_NONE,
                               -1,
                               NULL,
                               &error);

  if (error) {
    // call the error callback to notify client about the error condition
    std::cerr << "Error occured '" << error->message << "'\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR, std::string(error->message));
    return;
  }

  PostAsyncSuccessReply(msg);
}

void PhoneInstance::HandleAnswerCall(const picojson::value& msg) {
  std::cout << "HandleAnswerCall entered.\n";

  picojson::value::object o;
  GError *error = NULL;
  g_dbus_connection_call_sync( g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL),
                               PHONE_SERVICE,
                               PHONE_OBJ_PATH,
                               PHONE_IFACE,
                               "Answer",
                               NULL,
                               NULL,
                               G_DBUS_CALL_FLAGS_NONE,
                               -1,
                               NULL,
                               &error);

  if (error) {
    // call the error callback to notify client about the error condition
    std::cerr << "Error occured '" << error->message << "'\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR, std::string(error->message));
    return;
  }

  PostAsyncSuccessReply(msg);
}

void PhoneInstance::HandleHangupCall(const picojson::value& msg) {
  std::cout << "HandleHangupCall entered.\n";

  picojson::value::object o;
  GError *error = NULL;
  g_dbus_connection_call_sync( g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL),
                               PHONE_SERVICE,
                               PHONE_OBJ_PATH,
                               PHONE_IFACE,
                               "Hangup",
                               NULL,
                               NULL,
                               G_DBUS_CALL_FLAGS_NONE,
                               -1,
                               NULL,
                               &error);

  if (error) {
    // call the error callback to notify client about the error condition
    std::cerr << "Error occured '" << error->message << "'\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR, std::string(error->message));
    return;
  }

  PostAsyncSuccessReply(msg);
}

void PhoneInstance::HandleActiveCall(const picojson::value& msg) {
  std::cout << "HandleActiveCall entered.\n";

  GError *error = NULL;
  GVariant *reply = NULL;
  reply = g_dbus_connection_call_sync( g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL),
                                       PHONE_SERVICE,
                                       PHONE_OBJ_PATH,
                                       PHONE_IFACE,
                                       "ActiveCall",
                                       NULL,
                                       NULL,
                                       G_DBUS_CALL_FLAGS_NONE,
                                       -1,
                                       NULL,
                                       &error);

  if (error) {
    // call the error callback to notify client about the error condition
    std::cerr << "Error occured '" << error->message << "'\n";
    SendSyncErrorReply(UNKNOWN_ERR, std::string(error->message));
  } else if (!reply) {
    std::cerr << "No reply\n";
    SendSyncErrorReply(UNKNOWN_ERR);
  } else {
    picojson::value::object o;
    GVariantIter *iter;
    g_variant_get(reply, "(a{sv})", &iter);
    const char *key = NULL, *state = NULL, *line_id = NULL, *contact = NULL;
    GVariant *value;
    while (g_variant_iter_next(iter, "{sv}", &key, &value)) {
      if (!strcmp(key, "state")) {
        state = g_variant_get_string(value, NULL);
      }
      else if (!strcmp(key, "line_id")) {
        line_id = g_variant_get_string(value, NULL);
      }
      else if (!strcmp(key, "contact")) {
        contact = g_variant_get_string(value, NULL);
      }
    }

    o["state"] = state ? picojson::value(state) : picojson::value("");
    o["line_id"] = line_id ? picojson::value(line_id) : picojson::value("");
    o["contact"] = contact ? picojson::value(contact) : picojson::value("");
    picojson::value result(o);
    SendSyncSuccessReply(result);
  }

  if (reply)
    g_variant_unref(reply);
}

void PhoneInstance::HandleMuteCall(const picojson::value& msg) {
  std::cout << "HandleMuteCall entered.\n";
  if (!msg.contains("mute")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  picojson::value::object o;
  bool mute = msg.get("mute").evaluate_as_boolean();

  GError *error = NULL;
  g_dbus_connection_call_sync( g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL),
                               PHONE_SERVICE,
                               PHONE_OBJ_PATH,
                               PHONE_IFACE,
                               "Mute",
                               g_variant_new("(b)", mute),
                               NULL,
                               G_DBUS_CALL_FLAGS_NONE,
                               -1,
                               NULL,
                               &error);

  if (error) {
    // call the error callback to notify client about the error condition
    std::cerr << "Error occured '" << error->message << "'\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR, std::string(error->message));
    return;
  }

  PostAsyncSuccessReply(msg);
}

void PhoneInstance::HandleGetContacts(const picojson::value& msg) {
  std::cout << "HandleGetContacts entered.\n";
  if (!msg.contains("count")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  picojson::value::object o;
  int count = msg.get("count").get<double>();

  GError *error = NULL;
  GVariant *reply;
  reply = g_dbus_connection_call_sync( g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL),
                                       PHONE_SERVICE,
                                       PHONE_OBJ_PATH,
                                       PHONE_IFACE,
                                       "GetContacts",
                                       g_variant_new("(u)", count),
                                       NULL,
                                       G_DBUS_CALL_FLAGS_NONE,
                                       -1,
                                       NULL,
                                       &error);

  if (error) {
    // call the error callback to notify client about the error condition
    std::cerr << "Error occured '" << error->message << "'\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR, std::string(error->message));
  } else if (!reply) {
    std::cerr << "No reply\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR);
  } else {
    const char *result;
    g_variant_get(reply, "(s)", &result);
    if (result) {
      picojson::value value;
      std::string err;
      picojson::parse(value, result, result + strlen(result), &err);
      if (!err.empty()) {
        std::cout << "cannot parse result.\n";
        PostAsyncErrorReply(msg, UNKNOWN_ERR);
        return;
      }
      PostAsyncSuccessReply(msg, value);
    } else {
      PostAsyncErrorReply(msg, UNKNOWN_ERR);
    }
  }

  if (reply)
    g_variant_unref(reply);
}

void PhoneInstance::HandleGetCallHistory(const picojson::value& msg) {
  std::cout << "HandleGetCallHistory entered.\n";  
  if (!msg.contains("count")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  picojson::value::object o;
  int count = msg.get("count").get<double>();

  GError *error = NULL;
  GVariant *reply;
  reply = g_dbus_connection_call_sync( g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL),
                                       PHONE_SERVICE,
                                       PHONE_OBJ_PATH,
                                       PHONE_IFACE,
                                       "GetCallHistory",
                                       g_variant_new("(u)", count),
                                       NULL,
                                       G_DBUS_CALL_FLAGS_NONE,
                                       -1,
                                       NULL,
                                       &error);

  if (error) {
    // call the error callback to notify client about the error condition
    std::cerr << "Error occured '" << error->message << "'\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR, std::string(error->message));
  } else if (!reply) {
    std::cerr << "No reply\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR);
  } else {
    const char *result;
    g_variant_get(reply, "(s)", &result);
    if (result) {
      picojson::value value;
      std::string err;
      picojson::parse(value, result, result + strlen(result), &err);
      if (!err.empty()) {
        std::cout << "cannot parse result.\n";
        PostAsyncErrorReply(msg, UNKNOWN_ERR);
        return;
      }
      PostAsyncSuccessReply(msg, value);
    } else {
      PostAsyncErrorReply(msg, UNKNOWN_ERR);
    }
  }

  if (reply)
    g_variant_unref(reply);
}

void PhoneInstance::HandleAddRemoteDeviceSelectedListener(
  const picojson::value& msg) {
  std::cout << "AddRemoteDeviceSelectedListener entered.\n";
  picojson::value::object o;

  remoteDeviceSelectedListenerId_ = g_dbus_connection_signal_subscribe(
                                    g_bus_get_sync(G_BUS_TYPE_SESSION, NULL,NULL),
                                    PHONE_SERVICE,
                                    PHONE_IFACE,
                                    "RemoteDeviceSelected",
                                    NULL,
                                    NULL,
                                    G_DBUS_SIGNAL_FLAGS_NONE,
                                    handleSignal,
                                    this,
                                    NULL);
  if (remoteDeviceSelectedListenerId_ <= 0) {
    std::cerr << "Failed to subscribe for 'RemoteDeviceSelected': " << remoteDeviceSelectedListenerId_ << "\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR);
    return;
  }

  PostAsyncSuccessReply(msg);
}

void PhoneInstance::HandleRemoveRemoteDeviceSelectedListener(
  const picojson::value& msg) {
  std::cout << "RemoveRemoteDeviceSelectedListener entered.\n";
  picojson::value::object o;

  if (remoteDeviceSelectedListenerId_ == 0) {
    std::cerr << "Failed to unsubscribe for 'RemoteDeviceSelected'\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR);
    return;
  }

  g_dbus_connection_signal_unsubscribe(g_bus_get_sync(G_BUS_TYPE_SESSION, NULL,NULL),
                                       remoteDeviceSelectedListenerId_);
  remoteDeviceSelectedListenerId_ = 0;
  PostAsyncSuccessReply(msg);
}

void PhoneInstance::HandleAddCallChangedListener(
  const picojson::value& msg) {
  std::cout << "AddCallChangedListener entered.\n";
  picojson::value::object o;

  callChangedListenerId_ = g_dbus_connection_signal_subscribe(
                           g_bus_get_sync(G_BUS_TYPE_SESSION, NULL,NULL),
                           PHONE_SERVICE,
                           PHONE_IFACE,
                           "CallChanged",
                           NULL,
                           NULL,
                           G_DBUS_SIGNAL_FLAGS_NONE,
                           handleSignal,
                           this,
                           NULL);
  if (callChangedListenerId_ <= 0) {
    std::cerr << "Failed to subscribe for 'CallChanged': " << callChangedListenerId_ << "\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR);
    return;
  }

  PostAsyncSuccessReply(msg);
}

void PhoneInstance::HandleRemoveCallChangedListener(
  const picojson::value& msg) {
  std::cout << "RemoveCallChangedListener entered.\n";
  picojson::value::object o;

  if (callChangedListenerId_ == 0) {
    std::cerr << "Failed to unsubscribe for 'CallChanged'\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR);
    return;
  }

  g_dbus_connection_signal_unsubscribe(g_bus_get_sync(G_BUS_TYPE_SESSION, NULL,NULL),
                                       callChangedListenerId_);
  callChangedListenerId_ = 0;
  PostAsyncSuccessReply(msg);
}

void PhoneInstance::HandleAddCallHistoryEntryAddedListener(
  const picojson::value& msg) {
  std::cout << "AddCallHistoryEntryAddedListener entered.\n";
  picojson::value::object o;

  callHistoryEntryAddedListenerId_ = g_dbus_connection_signal_subscribe(
                                     g_bus_get_sync(G_BUS_TYPE_SESSION, NULL,NULL),
                                     PHONE_SERVICE,
                                     PHONE_IFACE,
                                     "CallHistoryEntryAddedChanged",
                                     NULL,
                                     NULL,
                                     G_DBUS_SIGNAL_FLAGS_NONE,
                                     handleSignal,
                                     this,
                                     NULL);
  if (callHistoryEntryAddedListenerId_ <= 0) {
    std::cerr << "Failed to subscribe for 'CallHistoryEntryAdded': " << callHistoryEntryAddedListenerId_ << "\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR);
    return;
  }

  PostAsyncSuccessReply(msg);
}

void PhoneInstance::HandleRemoveCallHistoryEntryAddedListener(
  const picojson::value& msg) {
  std::cout << "RemoveCallHistoryEntryAddedListener entered.\n";
  picojson::value::object o;

  if (callChangedListenerId_ == 0) {
    std::cerr << "Failed to unsubscribe for 'CallChanged'\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR);
    return;
  }

  g_dbus_connection_signal_unsubscribe(g_bus_get_sync(G_BUS_TYPE_SESSION, NULL,NULL),
                                       callHistoryEntryAddedListenerId_);
  callHistoryEntryAddedListenerId_ = 0;
  PostAsyncSuccessReply(msg);
}

void PhoneInstance::HandleAddCallHistoryChangedListener(
  const picojson::value& msg) {
  std::cout << "AddCallHistoryChangedListener entered.\n";
  picojson::value::object o;

  callHistoryChangedListenerId_ = g_dbus_connection_signal_subscribe(
                                  g_bus_get_sync(G_BUS_TYPE_SESSION, NULL,NULL),
                                  PHONE_SERVICE,
                                  PHONE_IFACE,
                                  "CallHistoryChanged",
                                  NULL,
                                  NULL,
                                  G_DBUS_SIGNAL_FLAGS_NONE,
                                  handleSignal,
                                  this,
                                  NULL);
  if (callHistoryChangedListenerId_ <= 0) {
    std::cerr << "Failed to subscribe for 'CallHistoryChanged': " << callHistoryChangedListenerId_ << "\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR);
    return;
  }

  PostAsyncSuccessReply(msg);
}

void PhoneInstance::HandleRemoveCallHistoryChangedListener(
  const picojson::value& msg) {
  std::cout << "RemoveCallHistoryChangedListener entered.\n";
  picojson::value::object o;

  if (callChangedListenerId_ == 0) {
    std::cerr << "Failed to unsubscribe for 'CallHistoryChanged'\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR);
    return;
  }

  g_dbus_connection_signal_unsubscribe(g_bus_get_sync(G_BUS_TYPE_SESSION, NULL,NULL),
                                       callHistoryChangedListenerId_);
  callHistoryChangedListenerId_ = 0;
  PostAsyncSuccessReply(msg);
}

void PhoneInstance::HandleAddContactsChangedListener(
  const picojson::value& msg) {
  std::cout << "AddContactsChangedListener entered.\n";
  picojson::value::object o;

  contactsChangedListenerId_ = g_dbus_connection_signal_subscribe(
                               g_bus_get_sync(G_BUS_TYPE_SESSION, NULL,NULL),
                               PHONE_SERVICE,
                               PHONE_IFACE,
                               "ContactsChanged",
                               NULL,
                               NULL,
                               G_DBUS_SIGNAL_FLAGS_NONE,
                               handleSignal,
                               this,
                               NULL);
  if (contactsChangedListenerId_ <= 0) {
    std::cerr << "Failed to subscribe for 'ContactsChanged': " << contactsChangedListenerId_ << "\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR);
    return;
  }

  PostAsyncSuccessReply(msg);
}

void PhoneInstance::HandleRemoveContactsChangedListener(
  const picojson::value& msg) {
  std::cout << "RemoveContactsChangedListener entered.\n";
  picojson::value::object o;

  if (contactsChangedListenerId_ == 0) {
    std::cerr << "Failed to unsubscribe for 'ContactsChanged'\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR);
    return;
  }

  g_dbus_connection_signal_unsubscribe(g_bus_get_sync(G_BUS_TYPE_SESSION, NULL,NULL),
                                       contactsChangedListenerId_);
  contactsChangedListenerId_ = 0;
  PostAsyncSuccessReply(msg);
}

guint PhoneInstance::remoteDeviceSelectedListenerId_ = 0;
guint PhoneInstance::callChangedListenerId_ = 0;
guint PhoneInstance::callHistoryEntryAddedListenerId_ = 0;
guint PhoneInstance::callHistoryChangedListenerId_ = 0;
guint PhoneInstance::contactsChangedListenerId_ = 0;
std::thread *PhoneInstance::t_ = NULL;

// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "phone/phone_instance.h"

#include <gio/gio.h>
#include <string>

namespace {

const char kPhoneService[] = "org.tizen.phone";
const char kPhoneInterface[] = "org.tizen.Phone";
const char kPhoneObjectPath[] = "/";

}  // namespace

guint PhoneInstance::remote_device_selected_listener_id_ = 0;
guint PhoneInstance::call_changed_listener_id_ = 0;
guint PhoneInstance::call_history_entry_added_listener_id_ = 0;
guint PhoneInstance::call_history_changed_listener_id_ = 0;
guint PhoneInstance::contacts_changed_listener_id_ = 0;

PhoneInstance::PhoneInstance()
    : main_loop_(g_main_loop_new(0, FALSE)),
      thread_(PhoneInstance::RunMainloop, this) {
  thread_.detach();
}

PhoneInstance::~PhoneInstance() {
  g_main_loop_quit(main_loop_);
}

void PhoneInstance::RunMainloop(void* data) {
  PhoneInstance* self = reinterpret_cast<PhoneInstance*>(data);
  GMainContext* ctx = g_main_context_default();
  g_main_context_push_thread_default(ctx);
  g_main_loop_run(self->main_loop_);
  g_main_loop_unref(self->main_loop_);
}

void PhoneInstance::HandleMessage(const char* msg) {
  picojson::value v;

  std::string err;
  picojson::parse(v, msg, msg + strlen(msg), &err);
  if (!err.empty()) {
    return;
  }

  const std::string cmd = v.get("cmd").to_str();
  if (cmd == "SelectRemoteDevice") {
    HandleSelectRemoteDevice(v);
  } else if (cmd == "UnselectRemoteDevice") {
    HandleUnselectRemoteDevice(v);
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
  } else if (cmd == "GetSelectedRemoteDevice" ||
             cmd == "GetContacts" ||
             cmd == "GetCallHistory") {
    HandleGet(cmd, v);
  } else if (cmd == "AddRemoteDeviceSelectedListener") {
    HandleAddListener(remote_device_selected_listener_id_,
        std::string("RemoteDeviceSelected"), v);
  } else if (cmd == "RemoveRemoteDeviceSelectedListener") {
    HandleRemoveListener(remote_device_selected_listener_id_,
        std::string("RemoteDeviceSelected"), v);
  } else if (cmd == "AddCallChangedListener") {
    HandleAddListener(call_changed_listener_id_,
        std::string("CallChanged"), v);
  } else if (cmd == "RemoveCallChangedListener") {
    HandleRemoveListener(call_changed_listener_id_,
        std::string("CallChanged"), v);
  } else if (cmd == "AddCallHistoryEntryAddedListener") {
    HandleAddListener(call_history_entry_added_listener_id_,
        std::string("CallHistoryEntryAdded"), v);
  } else if (cmd == "RemoveCallHistoryEntryAddedListener") {
    HandleRemoveListener(call_history_entry_added_listener_id_,
        std::string("CallHistoryEntryAdded"), v);
  } else if (cmd == "AddCallHistoryChangedListener") {
    HandleAddListener(call_history_changed_listener_id_,
        std::string("CallHistoryChanged"), v);
  } else if (cmd == "RemoveCallHistoryChangedListener") {
    HandleRemoveListener(call_history_changed_listener_id_,
        std::string("CallHistoryChanged"), v);
  } else if (cmd == "AddContactsChangedListener") {
    HandleAddListener(contacts_changed_listener_id_,
        std::string("ContactsChanged"), v);
  } else if (cmd == "RemoveContactsChangedListener") {
    HandleRemoveListener(contacts_changed_listener_id_,
        std::string("ContactsChanged"), v);
  } else {
    std::cerr << "Unknown command: " << cmd << "\n";
  }
}

void PhoneInstance::HandleSyncMessage(const char* msg) {
  picojson::value v;
  std::string err;

  picojson::parse(v, msg, msg + strlen(msg), &err);
  if (!err.empty()) {
    return;
  }

  const std::string cmd = v.get("cmd").to_str();
  if (cmd == "ActiveCall") {
    HandleActiveCall(v);
  } else {
    std::cerr << "ASSERT NOT REACHED.\n";
  }
}

void PhoneInstance::SendSyncErrorReply(WebApiAPIErrors error_code,
    const std::string& error_msg = "") {
  picojson::value::object o;
  o["isError"] = picojson::value(true);
  o["errorCode"] = picojson::value(static_cast<double>(error_code));
  if (!error_msg.empty())
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

void PhoneInstance::SendSyncSuccessReply(const picojson::value& value) {
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
    WebApiAPIErrors error_code, const std::string& error_msg = "") {
  picojson::value::object reply;
  reply["isError"] = picojson::value(true);
  reply["errorCode"] = picojson::value(static_cast<double>(error_code));
  if (!error_msg.empty())
    reply["errorMessage"] = picojson::value(error_msg.c_str());
  PostAsyncReply(msg, reply);
}

void PhoneInstance::PostAsyncSuccessReply(const picojson::value& msg,
    const picojson::value& value) {
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

void PhoneInstance::SendSignal(const picojson::value& signal_name,
    const picojson::value& signal_value) {
  picojson::value::object o;
  o["cmd"] = picojson::value("signal");
  o["signal_name"] = signal_name;
  o["signal_value"] = signal_value;
  picojson::value msg(o);
  PostMessage(msg.serialize().c_str());
}

GVariant* PhoneInstance::CallDBus(const gchar* method_name,
    GVariant* parameters,
    GError** error) {
  if (!method_name)
    return NULL;

  return g_dbus_connection_call_sync(
      g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL),
      kPhoneService,
      kPhoneObjectPath,
      kPhoneInterface,
      method_name,
      parameters,
      NULL,
      G_DBUS_CALL_FLAGS_NONE,
      -1,
      NULL,
      error);
}

void PhoneInstance::HandleSignal(GDBusConnection* connection,
    const gchar* sender_name,
    const gchar* object_path,
    const gchar* interface_name,
    const gchar* signal_name,
    GVariant* parameters,
    gpointer user_data) {
  PhoneInstance* instance = static_cast<PhoneInstance*>(user_data);
  if (!instance) {
    std::cerr << "Failed to cast to instance..." << "\n";
    return;
  }

  if (!strcmp(signal_name, "RemoteDeviceSelected") ||
      !strcmp(signal_name, "CallHistoryEntryAdded")) {
    const gchar* result;
    g_variant_get(parameters, "(s)", &result);
    if (!result)
      return;

    picojson::value value;
    std::string err;
    picojson::parse(value, result, result + strlen(result), &err);
    if (!err.empty()) {
      std::cerr << "cannot parse result.\n";
      return;
    }

    instance->SendSignal(picojson::value(signal_name), value);
  } else if (!strcmp(signal_name, "CallChanged")) {
    gchar* key = NULL;
    const gchar* state = NULL;
    const gchar* line_id = NULL;
    const gchar* contact = NULL;
    picojson::value contact_obj;
    std::string contact_err = "No contact info";
    GVariantIter* iter;
    GVariant* value;

    g_variant_get(parameters, "(a{sv})", &iter);
    while (g_variant_iter_next(iter, "{sv}", &key, &value)) {
      if (!strcmp(key, "state")) {
        state = g_variant_get_string(value, NULL);
      } else if (!strcmp(key, "line_id")) {
        line_id = g_variant_get_string(value, NULL);
      } else if (!strcmp(key, "contact")) {
        contact = g_variant_get_string(value, NULL);
        contact_err = picojson::parse(contact_obj, contact,
                                      contact + strlen(contact));
      }
      g_free(key);
      g_variant_unref(value);
    }
    picojson::value::object o;
    o["state"] = state ? picojson::value(state) : picojson::value("");
    o["line_id"] = line_id ? picojson::value(line_id) : picojson::value("");
    o["contact"] = contact_err.empty() ? picojson::value(contact_obj)
                                       : picojson::value("");
    picojson::value v(o);
    instance->SendSignal(picojson::value(signal_name), v);
  } else if (!strcmp(signal_name, "CallHistoryChanged") ||
             !strcmp(signal_name, "ContactsChanged")) {
    instance->SendSignal(picojson::value(signal_name), picojson::value(""));
  } else {
    std::cerr << "Unknown signal: " << signal_name << "\n";
  }
}

void PhoneInstance::HandleSelectRemoteDevice(
    const picojson::value& msg) {
  if (!msg.contains("address")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  GError* error = NULL;
  CallDBus("SelectRemoteDevice",
      g_variant_new("(s)", msg.get("address").to_str().c_str()),
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
  GError* error = NULL;
  CallDBus("UnselectRemoteDevice", NULL, &error);

  if (error) {
    // call the error callback to notify client about the error condition
    std::cerr << "Error occured '" << error->message << "'\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR, std::string(error->message));
    return;
  }

  PostAsyncSuccessReply(msg);
}

void PhoneInstance::HandleInvokeCall(const picojson::value& msg) {
  if (!msg.contains("phoneNumber")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  picojson::value::object o;
  GError* error = NULL;
  CallDBus("Dial",
      g_variant_new("(s)", msg.get("phoneNumber").to_str().c_str()),
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
  GError* error = NULL;
  CallDBus("Answer", NULL, &error);

  if (error) {
    // call the error callback to notify client about the error condition
    std::cerr << "Error occured '" << error->message << "'\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR, std::string(error->message));
    return;
  }

  PostAsyncSuccessReply(msg);
}

void PhoneInstance::HandleHangupCall(const picojson::value& msg) {
  GError* error = NULL;
  CallDBus("Hangup", NULL, &error);

  if (error) {
    // call the error callback to notify client about the error condition
    std::cerr << "Error occured '" << error->message << "'\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR, std::string(error->message));
    return;
  }

  PostAsyncSuccessReply(msg);
}

void PhoneInstance::HandleActiveCall(const picojson::value& msg) {
  GError* error = NULL;
  GVariant* reply = NULL;
  reply = CallDBus("ActiveCall", NULL, &error);

  if (error) {
    // call the error callback to notify client about the error condition
    std::cerr << "Error occured '" << error->message << "'\n";
    SendSyncErrorReply(UNKNOWN_ERR, std::string(error->message));
  } else if (!reply) {
    std::cerr << "No reply\n";
    SendSyncErrorReply(UNKNOWN_ERR);
  } else {
    picojson::value::object o;
    gchar* key = NULL;
    const gchar* state = NULL;
    const gchar* line_id = NULL;
    const gchar* contact = NULL;
    GVariantIter* iter;
    GVariant* value;

    g_variant_get(reply, "(a{sv})", &iter);
    while (g_variant_iter_next(iter, "{sv}", &key, &value)) {
      if (!strcmp(key, "state"))
        state = g_variant_get_string(value, NULL);
      else if (!strcmp(key, "line_id"))
        line_id = g_variant_get_string(value, NULL);
      else if (!strcmp(key, "contact"))
        contact = g_variant_get_string(value, NULL);

      g_free(key);
      g_variant_unref(value);
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
  if (!msg.contains("mute")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  GError* error = NULL;
  CallDBus("Mute",
      g_variant_new("(b)", msg.get("mute").evaluate_as_boolean()),
      &error);

  if (error) {
    // call the error callback to notify client about the error condition
    std::cerr << "Error occured '" << error->message << "'\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR, std::string(error->message));
    return;
  }

  PostAsyncSuccessReply(msg);
}

void PhoneInstance::HandleGet(const std::string& cmd,
    const picojson::value& msg) {
  GError* error = NULL;
  GVariant* reply = NULL;

  if (cmd == "GetSelectedRemoteDevice") {
    reply = CallDBus(cmd.c_str(), NULL, &error);
  } else if (cmd == "GetContacts" || cmd == "GetCallHistory") {
    if (!msg.contains("count")) {
      PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
      return;
    }

    reply = CallDBus(cmd.c_str(),
        g_variant_new("(u)", msg.get("count").get<double>()),
        &error);
  }

  if (error) {
    // call the error callback to notify client about the error condition
    std::cerr << "Error occured '" << error->message << "'\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR, std::string(error->message));
  } else if (!reply) {
    std::cerr << "No reply\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR);
  } else {
    const gchar* result;
    g_variant_get(reply, "(s)", &result);

    if (!result) {
      PostAsyncErrorReply(msg, UNKNOWN_ERR);
      return;
    }

    if (cmd == "GetSelectedRemoteDevice") {
      picojson::value value(result);
      PostAsyncSuccessReply(msg, value);
    } else if (cmd == "GetContacts" || cmd == "GetCallHistory") {
      picojson::value value;
      std::string err;
      picojson::parse(value, result, result + strlen(result), &err);
      if (!err.empty()) {
        std::cerr << "cannot parse result.\n";
        PostAsyncErrorReply(msg, UNKNOWN_ERR);
        g_variant_unref(reply);
        return;
      }
      PostAsyncSuccessReply(msg, value);
    } else {
      PostAsyncErrorReply(msg, UNKNOWN_ERR);
    }

    g_variant_unref(reply);
  }
}

void PhoneInstance::HandleAddListener(guint& listener_id,
    const std::string& signal_name,
    const picojson::value& msg) {
  listener_id = g_dbus_connection_signal_subscribe(
      g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL),
      kPhoneService,
      kPhoneInterface,
      signal_name.c_str(),
      NULL,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      HandleSignal,
      this,
      NULL);

  if (listener_id <= 0) {
    std::cerr << "Failed to subscribe for '" << signal_name << "': "
              << listener_id << "\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR);
    return;
  }

  PostAsyncSuccessReply(msg);
}

void PhoneInstance::HandleRemoveListener(guint& listener_id,
    const std::string& signal_name,
    const picojson::value& msg) {
  if (listener_id == 0) {
    std::cerr << "Failed to unsubscribe for '" << signal_name << "'\n";
    PostAsyncErrorReply(msg, UNKNOWN_ERR);
    return;
  }

  g_dbus_connection_signal_unsubscribe(
      g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL),
      listener_id);

  listener_id = 0;
  PostAsyncSuccessReply(msg);
}

// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PHONE_PHONE_INSTANCE_H_
#define PHONE_PHONE_INSTANCE_H_

#include <gio/gio.h>
#include <string>
#include <thread> // NOLINT

#include "common/extension.h"
#include "common/picojson.h"
#include "tizen/tizen.h"

class PhoneInstance : public common::Instance {
 public:
  PhoneInstance();
  ~PhoneInstance();

 private:
  // common::Instance implementation.
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* msg);

  // Synchronous messages
  void HandleActiveCall(const picojson::value& msg);

  // Asynchronous messages
  void HandleSelectRemoteDevice(const picojson::value& msg);
  void HandleUnselectRemoteDevice(const picojson::value& msg);
  void HandleInvokeCall(const picojson::value& msg);
  void HandleAnswerCall(const picojson::value& msg);
  void HandleHangupCall(const picojson::value& msg);
  void HandleMuteCall(const picojson::value& msg);
  void HandleGet(const std::string& cmd, const picojson::value& msg);
  void HandleAddListener(guint& listener_id,
                         const std::string& signal_name,
                         const picojson::value& msg);
  void HandleRemoveListener(guint& listener_id,
                            const std::string& signal_name,
                            const picojson::value& msg);

  // Synchronous message helpers
  void SendSyncErrorReply(WebApiAPIErrors error_code,
                          const std::string& error_msg);
  void SendSyncSuccessReply();
  void SendSyncSuccessReply(const picojson::value& value);

  // Asynchronous message helpers
  void PostAsyncReply(const picojson::value& msg,
                      picojson::value::object& value);
  void PostAsyncErrorReply(const picojson::value& msg,
                           WebApiAPIErrors error_code,
                           const std::string& error_msg);
  void PostAsyncSuccessReply(const picojson::value& msg,
                             const picojson::value& value);
  void PostAsyncSuccessReply(const picojson::value& msg);

  void SendSignal(const picojson::value& signal_name,
                  const picojson::value& signal_value);

  static GVariant* CallDBus(const gchar* method_name,
                            GVariant* parameters,
                            GError **error);

  static void HandleSignal(GDBusConnection* connection,
                           const gchar* sender_name,
                           const gchar* object_path,
                           const gchar* interface_name,
                           const gchar* signal_name,
                           GVariant* parameters,
                           gpointer user_data);

  static void RunMainloop(void* data);

  static guint remote_device_selected_listener_id_;
  static guint call_changed_listener_id_;
  static guint call_history_entry_added_listener_id_;
  static guint call_history_changed_listener_id_;
  static guint contacts_changed_listener_id_;

  GMainLoop* main_loop_;
  std::thread thread_;
};

#endif  // PHONE_PHONE_INSTANCE_H_

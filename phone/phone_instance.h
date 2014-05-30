// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PHONE_PHONE_INSTANCE_H_
#define PHONE_PHONE_INSTANCE_H_

#include "common/extension.h"
#include "common/picojson.h"
#include "tizen/tizen.h"
#include <gio/gio.h>

namespace std {
  class thread;
};

class PhoneInstance : public common::Instance {
 public:
  PhoneInstance();
  ~PhoneInstance();

 private:
  // common::Instance implementation.
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* msg);

  /* Synchronous messages */
  void HandleSelectRemoteDevice(const picojson::value& msg);
  void HandleUnselectRemoteDevice(const picojson::value& msg);
  void HandleGetSelectedRemoteDevice(const picojson::value& msg);
  void HandleInvokeCall(const picojson::value& msg);
  void HandleAnswerCall(const picojson::value& msg);
  void HandleHangupCall(const picojson::value& msg);
  void HandleActiveCall(const picojson::value& msg);
  void HandleMuteCall(const picojson::value& msg);
  void HandleGetContacts(const picojson::value& msg);
  void HandleGetCallHistory(const picojson::value& msg);
  void HandleAddRemoteDeviceSelectedListener(const picojson::value& msg);
  void HandleRemoveRemoteDeviceSelectedListener(const picojson::value& msg);
  void HandleAddCallChangedListener(const picojson::value& msg);
  void HandleRemoveCallChangedListener(const picojson::value& msg);
  void HandleAddCallHistoryEntryAddedListener(const picojson::value& msg);
  void HandleRemoveCallHistoryEntryAddedListener(const picojson::value& msg);
  void HandleAddCallHistoryChangedListener(const picojson::value& msg);
  void HandleRemoveCallHistoryChangedListener(const picojson::value& msg);
  void HandleAddContactsChangedListener(const picojson::value& msg);
  void HandleRemoveContactsChangedListener(const picojson::value& msg);

  /* Synchronous message helpers */
  void SendSyncErrorReply(WebApiAPIErrors);
  void SendSyncErrorReply(WebApiAPIErrors, const std::string&);
  void SendSyncSuccessReply();
  void SendSyncSuccessReply(const picojson::value);

  /* Asynchronous message helpers */
  void PostAsyncReply(const picojson::value&, picojson::value::object&);
  void PostAsyncErrorReply(const picojson::value&, WebApiAPIErrors);
  void PostAsyncErrorReply(const picojson::value&, WebApiAPIErrors, const std::string&);
  void PostAsyncSuccessReply(const picojson::value&, picojson::value&);
  void PostAsyncSuccessReply(const picojson::value&, WebApiAPIErrors);
  void PostAsyncSuccessReply(const picojson::value&);

  // OFono specific
  static void handleSignal(GDBusConnection *connection,
                           const gchar *sender_name,
                           const gchar *object_path,
                           const gchar *interface_name,
                           const gchar *signal_name,
                           GVariant *parameters,
                           gpointer user_data);

  static guint remoteDeviceSelectedListenerId_;
  static guint callChangedListenerId_;
  static guint callHistoryEntryAddedListenerId_;
  static guint callHistoryChangedListenerId_;
  static guint contactsChangedListenerId_;
  static std::thread *t_;
};

#endif  // PHONE_PHONE_INSTANCE_H_

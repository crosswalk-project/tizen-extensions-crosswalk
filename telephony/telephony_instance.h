// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TELEPHONY_TELEPHONY_INSTANCE_H_
#define TELEPHONY_TELEPHONY_INSTANCE_H_

#include <glib.h>
#include <thread>  // NOLINT

#include "common/extension.h"

namespace picojson {

class value;

}

class TelephonyBackend;

class TelephonyInstance : public common::Instance {
  friend class TelephonyBackend;
 public:
  TelephonyInstance();
  virtual ~TelephonyInstance();

 private:
  // common::Instance implementation.
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* msg);

  void SendSuccessReply(const picojson::value& msg);
  void SendSuccessReply(const picojson::value& msg,
      const picojson::value& value);
  void SendErrorReply(const picojson::value& msg,
      const int error_code, const char* error_msg = NULL);
  void SendNotification(const picojson::value& msg);

 private:
  std::thread dbus_thread_;
  GMainLoop* dbus_loop_;
  TelephonyBackend* backend_;
};

#endif  // TELEPHONY_TELEPHONY_INSTANCE_H_

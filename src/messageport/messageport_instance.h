// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MESSAGEPORT_MESSAGEPORT_INSTANCE_H_
#define MESSAGEPORT_MESSAGEPORT_INSTANCE_H_

#include <bundle.h>

#include <map>

#include "common/extension.h"
#include "common/picojson.h"

class MessageportInstance : public common::Instance {
 private:
  // common::Instance implementation.
  virtual void HandleMessage(const char*) {}
  virtual void HandleSyncMessage(const char* msg);

  // Command handlers.
  void HandleRequestLocalMessagePort(const picojson::value& msg,
          picojson::value::object& reply);
  void HandleRequestRemoteMessagePort(const picojson::value& msg,
          picojson::value::object& reply);
  void HandleSendMessage(const picojson::value& msg,
          picojson::value::object& reply);

  // Messageport ID <-> MessageportInstance mapping.
  typedef std::map<int, MessageportInstance *> MessageportIdToInstanceMap;
  static MessageportIdToInstanceMap mp_id_to_instance_map_;

  static void RegisterLocalMessageport(int mp_id,
        MessageportInstance *instance);
  static MessageportInstance* GetInstanceByPortId(int mp_id);

  // bundle_iterate() callback.
  static void BundleJsonIterator(const char *key, const char *value,
        void *data);

  // messageport_register[_trusted]_local_port() implementation.
  void OnReceiveLocalMessage(int id, const char* remote_app_id,
        const char* remote_port, bool trusted_message, bundle* data);
  static void OnReceiveLocalMessageThunk(int id, const char* remote_app_id,
        const char* remote_port, bool trusted_message, bundle* data);
};

#endif  // MESSAGEPORT_MESSAGEPORT_INSTANCE_H_

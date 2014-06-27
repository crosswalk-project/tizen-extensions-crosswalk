// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VEHICLE_VEHICLE_H_
#define VEHICLE_VEHICLE_H_

#include <abstractpropertytype.h>
#include <gio/gio.h>
#include <glib.h>

#include <string>
#include <thread> // NOLINT

#include "common/picojson.h"

namespace common {

class Instance;

}  // namespace common

typedef std::function<void (picojson::object)> GetReply;
typedef std::function<void (std::string)> ErrorReply;

class Vehicle {
 public:
  struct CallbackInfo {
    std::string method;
    common::Instance* instance;
    double callback_id;
  };

  explicit Vehicle(common::Instance* i);
  ~Vehicle();

  void Get(const std::string& property, Zone::Type zone, double ret_id);

 private:
  std::string FindProperty(const std::string& object_name, int zone);
  GDBusProxy* GetAutomotiveManager();

  static void SetupMainloop(void *data);
  GMainLoop* main_loop_;
  std::thread thread_;
  common::Instance* instance_;
};

#endif  // VEHICLE_VEHICLE_H_

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_DEVICE_ORIENTATION_H_
#define SYSTEM_INFO_SYSTEM_INFO_DEVICE_ORIENTATION_H_

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"

class SysInfoDeviceOrientation {
 public:
  static SysInfoDeviceOrientation& GetSysInfoDeviceOrientation(
      ContextAPI* api) {
    static SysInfoDeviceOrientation instance(api);
    return instance;
  }
  ~SysInfoDeviceOrientation() { }
  void Get(picojson::value& error, picojson::value& data);
  void StartListen() { };
  void StopListen() { };

 private:
  SysInfoDeviceOrientation(ContextAPI* api) {
    api_ = api;
  }

  ContextAPI* api_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoDeviceOrientation);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_DEVICE_ORIENTATION_H_

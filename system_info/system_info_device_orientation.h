// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_DEVICE_ORIENTATION_H_
#define SYSTEM_INFO_SYSTEM_INFO_DEVICE_ORIENTATION_H_

#if defined(TIZEN_MOBILE)
#include <sensor.h>
#include <vconf.h>
#endif

#include <string>

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "common/utils.h"

enum SystemInfoDeviceOrientationStatus {
  PORTRAIT_PRIMARY,
  PORTRAIT_SECONDARY,
  LANDSCAPE_PRIMARY,
  LANDSCAPE_SECONDARY,
  UNKNOWN
};

class SysInfoDeviceOrientation {
 public:
  static SysInfoDeviceOrientation& GetSysInfoDeviceOrientation(
      ContextAPI* api) {
    static SysInfoDeviceOrientation instance(api);
    return instance;
  }
  ~SysInfoDeviceOrientation() { }
  void Get(picojson::value& error, picojson::value& data);
  void StartListening();
  void StopListening();

 private:
  explicit SysInfoDeviceOrientation(ContextAPI* api)
    :status_(UNKNOWN) {
    api_ = api;
  }

#if defined(TIZEN_MOBILE)
  void SetStatus();
  bool SetAutoRotation();
  void SendUpdate();
  void SetData(picojson::value& data);
  std::string ToOrientationStatusString
      (SystemInfoDeviceOrientationStatus status);
  enum SystemInfoDeviceOrientationStatus EventToStatus(int event_data);
  static void OnAutoRotationChanged(keynode_t* node, void* user_data);
  static void OnDeviceOrientationChanged(unsigned int event_type,
                                         sensor_event_data_t* event,
                                         void* data);
#endif

  ContextAPI* api_;
  SystemInfoDeviceOrientationStatus status_;
  bool isAutoRotation_;
  int sensorHandle_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoDeviceOrientation);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_DEVICE_ORIENTATION_H_

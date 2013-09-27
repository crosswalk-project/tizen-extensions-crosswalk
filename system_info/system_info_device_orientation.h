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
#include "system_info/system_info_utils.h"

enum SystemInfoDeviceOrientationStatus {
  PORTRAIT_PRIMARY,
  PORTRAIT_SECONDARY,
  LANDSCAPE_PRIMARY,
  LANDSCAPE_SECONDARY,
};

class SysInfoDeviceOrientation {
 public:
  static SysInfoDeviceOrientation& GetSysInfoDeviceOrientation() {
    static SysInfoDeviceOrientation instance;
    return instance;
  }
  ~SysInfoDeviceOrientation() {
    for (SystemInfoEventsList::iterator it = device_orientation_events_.begin();
         it != device_orientation_events_.end(); it++)
      StopListening(*it);
    pthread_mutex_destroy(&events_list_mutex_);
  }
  void Get(picojson::value& error, picojson::value& data);
  void StartListening(ContextAPI* api);
  void StopListening(ContextAPI* api);

 private:
  explicit SysInfoDeviceOrientation()
      : status_(PORTRAIT_PRIMARY),
        sensorHandle_(0) {
    pthread_mutex_init(&events_list_mutex_, NULL);
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

  SystemInfoDeviceOrientationStatus status_;
  bool isAutoRotation_;
  int sensorHandle_;
  pthread_mutex_t events_list_mutex_;

  DISALLOW_COPY_AND_ASSIGN(SysInfoDeviceOrientation);
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_DEVICE_ORIENTATION_H_

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_device_orientation.h"

const std::string SysInfoDeviceOrientation::name_ = "DEVICE_ORIENTATION";

void SysInfoDeviceOrientation::Get(picojson::value& error,
                                   picojson::value& data) {
  SetStatus();
  if (!SetAutoRotation()) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Set autoRotation state failed"));
    return;
  }
  SetData(data);
  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

void SysInfoDeviceOrientation::SetStatus() {
  unsigned long event;  // NOLINT

  int r = sf_check_rotation(&event);
  if (r < 0) {
    status_ = PORTRAIT_PRIMARY;
    return;
  }

  status_ = EventToStatus(event);
}

bool SysInfoDeviceOrientation::SetAutoRotation() {
  int value = 0;
  if (vconf_get_bool(VCONFKEY_SETAPPL_AUTO_ROTATE_SCREEN_BOOL, &value) != 0) {
    return false;
  }
  isAutoRotation_ = value == 1;
  return true;
}

void SysInfoDeviceOrientation::SetData(picojson::value& data) {
  system_info::SetPicoJsonObjectValue(data, "status",
      picojson::value(ToOrientationStatusString(status_)));
  system_info::SetPicoJsonObjectValue(data, "isAutoRotation",
      picojson::value(isAutoRotation_));
}

void SysInfoDeviceOrientation::SendUpdate() {
  picojson::value output = picojson::value(picojson::object());
  picojson::value data = picojson::value(picojson::object());

  SetData(data);
  system_info::SetPicoJsonObjectValue(output, "cmd",
      picojson::value("SystemInfoPropertyValueChanged"));
  system_info::SetPicoJsonObjectValue(output, "prop",
      picojson::value("DEVICE_ORIENTATION"));
  system_info::SetPicoJsonObjectValue(output, "data", data);

  PostMessageToListeners(output);
}

std::string SysInfoDeviceOrientation::ToOrientationStatusString(
    SystemInfoDeviceOrientationStatus status) {
  std::string ret;
  switch (status) {
    case PORTRAIT_PRIMARY:
      ret = "PORTRAIT_PRIMARY";
      break;
    case PORTRAIT_SECONDARY:
      ret = "PORTRAIT_SECONDARY";
      break;
    case LANDSCAPE_PRIMARY:
      ret = "LANDSCAPE_PRIMARY";
      break;
    case LANDSCAPE_SECONDARY:
      ret = "LANDSCAPE_SECONDARY";
      break;
    default:
      ret = "PORTRAIT_PRIMARY";
  }
  return ret;
}

enum SystemInfoDeviceOrientationStatus
SysInfoDeviceOrientation::EventToStatus(int event_data) {
  enum SystemInfoDeviceOrientationStatus m = PORTRAIT_PRIMARY;
  switch (event_data) {
    case(ROTATION_EVENT_0):
      m = PORTRAIT_PRIMARY;
      break;
    case(ROTATION_EVENT_90):
      m = LANDSCAPE_SECONDARY;
      break;
    case(ROTATION_EVENT_180):
      m = PORTRAIT_SECONDARY;
      break;
    case(ROTATION_EVENT_270):
      m = LANDSCAPE_PRIMARY;
      break;
    default:
      m = PORTRAIT_PRIMARY;
  }

  return m;
}

void
SysInfoDeviceOrientation::OnDeviceOrientationChanged(unsigned int event_type,
                                                     sensor_event_data_t* event,
                                                     void* data) {
  if (event_type != ACCELEROMETER_EVENT_ROTATION_CHECK) {
    return;
  }

  int* cb_event_data = reinterpret_cast<int*>(event->event_data);
  SysInfoDeviceOrientation* orientation =
      static_cast<SysInfoDeviceOrientation*>(data);

  enum SystemInfoDeviceOrientationStatus m =
      orientation->EventToStatus(*cb_event_data);
  if (m != orientation->status_) {
    orientation->status_ = m;
    orientation->SendUpdate();
  }
}

void SysInfoDeviceOrientation::OnAutoRotationChanged(keynode_t* node,
                                                     void* user_data) {
  int autoRotation = vconf_keynode_get_bool(node);

  SysInfoDeviceOrientation* orientation =
      static_cast<SysInfoDeviceOrientation*>(user_data);

  orientation->isAutoRotation_ = autoRotation == 1;
  orientation->SendUpdate();
}

void SysInfoDeviceOrientation::StartListening() {
  vconf_notify_key_changed(VCONFKEY_SETAPPL_AUTO_ROTATE_SCREEN_BOOL,
      (vconf_callback_fn)OnAutoRotationChanged, this);

  sensorHandle_ = sf_connect(ACCELEROMETER_SENSOR);
  if (sensorHandle_ < 0)
    return;

  int r = sf_register_event(sensorHandle_, ACCELEROMETER_EVENT_ROTATION_CHECK,
                            NULL, OnDeviceOrientationChanged, this);
  if (r < 0) {
    sf_disconnect(sensorHandle_);
    return;
  }

  r = sf_start(sensorHandle_, 0);
  if (r < 0) {
    sf_unregister_event(sensorHandle_, ACCELEROMETER_EVENT_ROTATION_CHECK);
    sf_disconnect(sensorHandle_);
  }
}

void SysInfoDeviceOrientation::StopListening() {
  vconf_ignore_key_changed(VCONFKEY_SETAPPL_AUTO_ROTATE_SCREEN_BOOL,
      (vconf_callback_fn)OnAutoRotationChanged);

  sf_unregister_event(sensorHandle_, ACCELEROMETER_EVENT_ROTATION_CHECK);
  sf_stop(sensorHandle_);
  sf_disconnect(sensorHandle_);
}

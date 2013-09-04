// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_device_orientation.h"

#include "system_info/system_info_utils.h"

void SysInfoDeviceOrientation::Get(picojson::value& error,
                                   picojson::value& data) {
  system_info::SetPicoJsonObjectValue(error, "message",
      picojson::value("Device Orientation is not supported on desktop."));
}

void SysInfoDeviceOrientation::StartListening() { }
void SysInfoDeviceOrientation::StopListening() { }

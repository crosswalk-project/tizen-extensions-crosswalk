// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_peripheral.h"

const std::string SysInfoPeripheral::name_ = "PERIPHERAL";

void SysInfoPeripheral::Get(picojson::value& error,
                            picojson::value& data) {
  system_info::SetPicoJsonObjectValue(error, "message",
      picojson::value("Peripheral is not supported on desktop."));
}

void SysInfoPeripheral::StartListening() { }
void SysInfoPeripheral::StopListening() { }

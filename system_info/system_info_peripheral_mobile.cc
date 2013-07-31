// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_peripheral.h"

#include "system_info/system_info_utils.h"

using namespace system_info;

void SysInfoPeripheral::Get(picojson::value& error,
                            picojson::value& data) {
  // FIXME(halton): Add actual implementation
  SetPicoJsonObjectValue(data, "isVideoOutputOn", picojson::value(true));
  SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

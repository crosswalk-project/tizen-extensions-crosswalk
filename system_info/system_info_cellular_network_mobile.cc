// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_cellular_network.h"

#include "system_info/system_info_utils.h"

using namespace system_info;

void SysInfoCellularNetwork::Get(picojson::value& error,
                                 picojson::value& data) {
  // FIXME(halton): Add actual implementation
  SetPicoJsonObjectValue(error, "message",
      picojson::value("NOT IMPLEMENTED."));
}

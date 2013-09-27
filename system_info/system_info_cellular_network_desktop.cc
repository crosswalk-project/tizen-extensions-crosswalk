// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_cellular_network.h"

void SysInfoCellularNetwork::Get(picojson::value& error,
                                 picojson::value& data) {
  system_info::SetPicoJsonObjectValue(error, "message",
      picojson::value("Cellular Network is not supported on desktop."));
}

void SysInfoCellularNetwork::StartListening(ContextAPI* api) { }
void SysInfoCellularNetwork::StopListening(ContextAPI* api) { }

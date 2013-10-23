// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_cellular_network.h"

const std::string SysInfoCellularNetwork::name_ = "CELLULAR_NETWORK";

void SysInfoCellularNetwork::Get(picojson::value& error,
                                 picojson::value& data) {
  system_info::SetPicoJsonObjectValue(error, "message",
      picojson::value("Cellular Network is not supported on desktop."));
}

void SysInfoCellularNetwork::AddListener(ContextAPI* api) { }
void SysInfoCellularNetwork::RemoveListener(ContextAPI* api) { }

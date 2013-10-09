// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_sim.h"

const std::string SysInfoSim::name_ = "SIM";

void SysInfoSim::Get(picojson::value& error,
                     picojson::value& data) {
  system_info::SetPicoJsonObjectValue(error, "message",
      picojson::value("SIM is not supported on desktop."));
}

void SysInfoSim::AddListener(ContextAPI* api) { }
void SysInfoSim::RemoveListener(ContextAPI* api) { }

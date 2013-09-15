// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_cpu.h"

#include <stdlib.h>
#include <string>

#include "system_info/system_info_utils.h"

void SysInfoCpu::Get(picojson::value& error, //NOLINT
                     picojson::value& data) {
  if (!UpdateLoad()) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get CPU load failed."));
    return;
  }

  system_info::SetPicoJsonObjectValue(data, "load", picojson::value(load_));
  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

gboolean SysInfoCpu::OnUpdateTimeout(gpointer user_data) {
  SysInfoCpu* instance = static_cast<SysInfoCpu*>(user_data);

  double old_load = instance->load_;
  instance->UpdateLoad();
  if (old_load != instance->load_) {
    picojson::value output = picojson::value(picojson::object());;
    picojson::value data = picojson::value(picojson::object());

    system_info::SetPicoJsonObjectValue(data, "load",
        picojson::value(instance->load_));
    system_info::SetPicoJsonObjectValue(output, "cmd",
        picojson::value("SystemInfoPropertyValueChanged"));
    system_info::SetPicoJsonObjectValue(output, "prop", picojson::value("CPU"));
    system_info::SetPicoJsonObjectValue(output, "data", data);

    std::string result = output.serialize();
    instance->api_->PostMessage(result.c_str());
  }

  return TRUE;
}

bool SysInfoCpu::UpdateLoad() {
  double loads[1];
  if (-1 == getloadavg(loads, 1)) {
    return false;
  }

  load_ = loads[0];
  return true;
}

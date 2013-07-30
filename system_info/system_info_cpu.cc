// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_cpu.h"

#include <stdlib.h>

#include "system_info/system_info_utils.h"

using namespace system_info;

void SysInfoCpu::Get(picojson::value& error,
                     picojson::value& data) {
  if (!UpdateLoad()) {
    SetPicoJsonObjectValue(error, "message",
        picojson::value("Get CPU load failed."));
    return;
  }

  SetPicoJsonObjectValue(data, "load", picojson::value(load_));
  SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

gboolean SysInfoCpu::TimedOutUpdate(gpointer user_data) {
  SysInfoCpu* instance = static_cast<SysInfoCpu*>(user_data);

  if (instance->stopping_) {
    instance->stopping_ = false;
    return FALSE;
  }

  double old_load = instance->load_;
  instance->UpdateLoad();
  if (old_load != instance->load_) {
    picojson::value output = picojson::value(picojson::object());;
    picojson::value data = picojson::value(picojson::object());

    SetPicoJsonObjectValue(data, "load", picojson::value(instance->load_));
    SetPicoJsonObjectValue(output, "cmd",
        picojson::value("SystemInfoPropertyValueChanged"));
    SetPicoJsonObjectValue(output, "prop", picojson::value("CPU"));
    SetPicoJsonObjectValue(output, "data", data);

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

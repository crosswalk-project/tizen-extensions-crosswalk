// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_sim.h"

#include "system_info/system_info_utils.h"

void SysInfoSim::Get(picojson::value& error,
                     picojson::value& data) {
  // FIXME(halton): Add actual implementation
  system_info::SetPicoJsonObjectValue(data, "state",
      picojson::value("READY"));
  system_info::SetPicoJsonObjectValue(data, "operatorName",
      picojson::value("China Mobile"));
  system_info::SetPicoJsonObjectValue(data, "msisdn",
      picojson::value("12321312"));
  system_info::SetPicoJsonObjectValue(data, "iccid",
      picojson::value("234234234"));
  system_info::SetPicoJsonObjectValue(data, "mcc",
      picojson::value(static_cast<double>(50)));
  system_info::SetPicoJsonObjectValue(data, "mnc",
      picojson::value(static_cast<double>(51)));
  system_info::SetPicoJsonObjectValue(data, "msin",
      picojson::value("China Mobile - msin"));
  system_info::SetPicoJsonObjectValue(data, "spn",
      picojson::value("China Mobile - spn"));
  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

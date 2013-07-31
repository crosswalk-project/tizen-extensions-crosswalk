// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_sim.h"

#include "system_info/system_info_utils.h"

using namespace system_info;

void SysInfoSim::Get(picojson::value& error,
                     picojson::value& data) {
  // FIXME(halton): Add actual implementation
  SetPicoJsonObjectValue(data, "state", picojson::value("READY"));
  SetPicoJsonObjectValue(data, "operatorName", picojson::value("China Mobile"));
  SetPicoJsonObjectValue(data, "msisdn", picojson::value("12321312"));
  SetPicoJsonObjectValue(data, "iccid", picojson::value("234234234"));
  SetPicoJsonObjectValue(data, "mcc", picojson::value(static_cast<double>(50)));
  SetPicoJsonObjectValue(data, "mnc", picojson::value(static_cast<double>(51)));
  SetPicoJsonObjectValue(data, "msin", picojson::value("China Mobile - msin"));
  SetPicoJsonObjectValue(data, "spn", picojson::value("China Mobile - spn"));
  SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

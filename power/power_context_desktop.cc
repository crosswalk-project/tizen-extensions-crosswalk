// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "power/power_context.h"
#include "common/picojson.h"

void PowerContext::Initialize() {
}

void PowerContext::HandleRequest(const picojson::value& msg) {
  std::string resource = msg.get("resource").to_str();
  std::string state = msg.get("state").to_str();
}

void PowerContext::HandleRelease(const picojson::value& msg) {
  std::string resource = msg.get("resource").to_str();
}

void PowerContext::HandleSetScreenBrightness(const picojson::value& msg) {
  double brightness = msg.get("value").get<double>();
}

void PowerContext::HandleSetScreenEnabled(const picojson::value& msg) {
  bool isEnabled = msg.get("value").get<bool>();
}

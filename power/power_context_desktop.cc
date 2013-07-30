// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "power/power_context.h"
#include "common/picojson.h"

static double kBrightness = 1;

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
  kBrightness = msg.get("value").get<double>();
}

void PowerContext::HandleGetScreenBrightness() {
  char brightnessAsString[32];
  snprintf(brightnessAsString, 32, "%g", kBrightness);
  api_->SetSyncReply(brightnessAsString);
}

void PowerContext::HandleSetScreenEnabled(const picojson::value& msg) {
  bool isEnabled = msg.get("value").get<bool>();
}

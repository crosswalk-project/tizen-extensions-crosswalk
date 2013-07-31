// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_context.h"

#include <stdlib.h>

#include "common/picojson.h"
#include "system_info/system_info_utils.h"

using namespace system_info;

CXWalkExtension* xwalk_extension_init(int32_t api_version) {
  return ExtensionAdapter<SystemInfoContext>::Initialize();
}

SystemInfoContext::SystemInfoContext(ContextAPI* api)
    : api_(api),
      battery_(SysInfoBattery::GetSysInfoBattery(api)),
      build_(SysInfoBuild::GetSysInfoBuild(api)),
      cpu_(SysInfoCpu::GetSysInfoCpu(api)),
      display_(SysInfoDisplay::GetSysInfoDisplay(api)),
      locale_(SysInfoLocale::GetSysInfoLocale(api)),
      storage_(SysInfoStorage::GetSysInfoStorage(api)) {
}

SystemInfoContext::~SystemInfoContext() {
  delete api_;
}

const char SystemInfoContext::name[] = "tizen.systeminfo";

// This will be generated from system_info_api.js.
extern const char kSource_system_info_api[];

const char* SystemInfoContext::GetJavaScript() {
  return kSource_system_info_api;
}

void SystemInfoContext::HandleGetPropertyValue(const picojson::value& input,
                                               picojson::value& output) {
  std::string reply_id = input.get("_reply_id").to_str();
  SetPicoJsonObjectValue(output, "_reply_id", picojson::value(reply_id));

  picojson::value error = picojson::value(picojson::object());
  picojson::value data = picojson::value(picojson::object());

  SetPicoJsonObjectValue(error, "message", picojson::value(""));
  std::string prop = input.get("prop").to_str();

  if (prop == "BATTERY") {
    battery_.Get(error, data);
  } else if (prop == "CPU") {
    cpu_.Get(error, data);
  } else if (prop == "STORAGE") {
    storage_.Get(error, data);
  } else if (prop == "DISPLAY") {
    display_.Get(error, data);
  } else if (prop == "DEVICE_ORIENTATION ") {
    GetDeviceOrientation(error, data);
  } else if (prop == "BUILD") {
    build_.Get(error, data);
  } else if (prop == "LOCALE") {
    locale_.Get(error, data);
  } else if (prop == "NETWORK") {
    GetNetwork(error, data);
  } else if (prop == "WIFI_NETWORK") {
    GetWifiNetwork(error, data);
  } else if (prop == "CELLULAR_NETWORK") {
    GetCellularNetwork(error, data);
  } else if (prop == "SIM") {
    GetSIM(error, data);
  } else if (prop == "PERIPHERAL") {
    GetPeripheral(error, data);
  } else {
    SetPicoJsonObjectValue(error, "message",
        picojson::value("Not supportted property " + prop));
  }

  if (!error.get("message").to_str().empty()) {
    SetPicoJsonObjectValue(output, "error", error);
  } else {
    SetPicoJsonObjectValue(output, "data", data);
  }

  std::string result = output.serialize();
  api_->PostMessage(result.c_str());
}

void SystemInfoContext::HandleStartListen(const picojson::value& input) {
  std::string prop = input.get("prop").to_str();

  if (prop == "BATTERY") {
    battery_.StartListen();
  } else if (prop == "CPU") {
    cpu_.StartListen();
  } else if (prop == "STORAGE") {
    storage_.StartListen();
  } else if (prop == "DISPLAY") {
    display_.StartListen();
  } else if (prop == "DEVICE_ORIENTATION ") {
    // FIXME(halton): Add DEVICE_ORIENTATION listener
  } else if (prop == "BUILD") {
    build_.StartListen();
  } else if (prop == "LOCALE") {
    locale_.StartListen();
  } else if (prop == "NETWORK") {
    // FIXME(halton): Add NETWORK listener
  } else if (prop == "WIFI_NETWORK") {
    // FIXME(halton): Add WIFI_NETWORK listener
  } else if (prop == "CELLULAR_NETWORK") {
    // FIXME(halton): Add CELLULAR_NETWORK listener
  } else if (prop == "SIM") {
    // FIXME(halton): Add SIM listener
  } else if (prop == "PERIPHERAL") {
    // FIXME(halton): Add PERIPHERAL listener
  } 
}

void SystemInfoContext::HandleStopListen(const picojson::value& input) {
  std::string prop = input.get("prop").to_str();

  if (prop == "BATTERY") {
    battery_.StopListen();
  } else if (prop == "CPU") {
    cpu_.StopListen();
  } else if (prop == "STORAGE") {
    storage_.StopListen();
  } else if (prop == "DISPLAY") {
    display_.StopListen();
  } else if (prop == "DEVICE_ORIENTATION ") {
    // FIXME(halton): Remove DEVICE_ORIENTATION listener
  } else if (prop == "BUILD") {
    build_.StopListen();
  } else if (prop == "LOCALE") {
    locale_.StopListen();
  } else if (prop == "NETWORK") {
    // FIXME(halton): Remove NETWORK listener
  } else if (prop == "WIFI_NETWORK") {
    // FIXME(halton): Remove WIFI_NETWORK listener
  } else if (prop == "CELLULAR_NETWORK") {
    // FIXME(halton): Remove CELLULAR_NETWORK listener
  } else if (prop == "SIM") {
    // FIXME(halton): Remove SIM listener
  } else if (prop == "PERIPHERAL") {
    // FIXME(halton): Remove PERIPHERAL listener
  } 
}

void SystemInfoContext::HandleMessage(const char* message) {
  picojson::value input;
  std::string err;

  picojson::parse(input, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }

  std::string cmd = input.get("cmd").to_str();
  if (cmd == "getPropertyValue") {
    picojson::value output = picojson::value(picojson::object());
    HandleGetPropertyValue(input, output);
  } else if (cmd == "startListen") {
    HandleStartListen(input);
  } else if (cmd == "stopListen") {
    HandleStopListen(input);
  }
}

void SystemInfoContext::HandleSyncMessage(const char*) {
}

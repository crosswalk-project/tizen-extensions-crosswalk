// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_context.h"

#include <stdlib.h>

#include "common/picojson.h"
#include "system_info/system_info_battery.h"
#include "system_info/system_info_display.h"
#include "system_info/system_info_storage.h"
#include "system_info/system_info_utils.h"

using namespace system_info;

CXWalkExtension* xwalk_extension_init(int32_t api_version) {
  return ExtensionAdapter<SystemInfoContext>::Initialize();
}

SystemInfoContext::SystemInfoContext(ContextAPI* api)
    : api_(api) {}

SystemInfoContext::~SystemInfoContext() {
  delete api_;
}

const char SystemInfoContext::name[] = "tizen.systeminfo";

// This will be generated from system_info_api.js.
extern const char kSource_system_info_api[];

const char* SystemInfoContext::GetJavaScript() {
  return kSource_system_info_api;
}

void SystemInfoContext::GetBattery(picojson::value& error,
                                   picojson::value& data) {
  SysInfoBattery& b = SysInfoBattery::GetSysInfoBattery(error);
  if (error.get("message").to_str().empty())
    b.Update(error, data);
}

void SystemInfoContext::GetCPU(picojson::value& error,
                               picojson::value& data) {
  double load[1];
  if (getloadavg(load, 1) == -1) {  
    SetPicoJsonObjectValue(error, "message",
        picojson::value("Get CPU load failed."));
    return;
  }

  SetPicoJsonObjectValue(data, "load", picojson::value(load[0]));
  SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

void SystemInfoContext::GetStorage(picojson::value& error,
                                   picojson::value& data) {
  SysInfoStorage& s = SysInfoStorage::GetSysInfoStorage(error);

  if (error.get("message").to_str().empty())
    s.Update(error, data);
}

void SystemInfoContext::GetDisplay(picojson::value& error,
                                   picojson::value& data) {
  double ret;
  SysInfoDisplay& disp = SysInfoDisplay::GetSysInfoDisplay();

  SetPicoJsonObjectValue(error, "message", picojson::value(""));
  ret = static_cast<double>(disp.GetResolutionWidth(error));
  if (DidFail(error))
    return;
  SetPicoJsonObjectValue(data, "resolutionWidth", picojson::value(ret));

  ret = static_cast<double>(disp.GetResolutionHeight(error));
  if (DidFail(error))
    return;
  SetPicoJsonObjectValue(data, "resolutionHeight", picojson::value(ret));

  ret = static_cast<double>(disp.GetDotsPerInchWidth(error));
  if (DidFail(error))
    return;
  SetPicoJsonObjectValue(data, "dotsPerInchWidth", picojson::value(ret));

  ret = static_cast<double>(disp.GetDotsPerInchWidth(error));
  if (DidFail(error))
    return;
  SetPicoJsonObjectValue(data, "dotsPerInchWidth", picojson::value(ret));

  ret = static_cast<double>(disp.GetDotsPerInchHeight(error));
  if (DidFail(error))
    return;
  SetPicoJsonObjectValue(data, "dotsPerInchHeight", picojson::value(ret));

  ret = static_cast<double>(disp.GetPhysicalWidth(error));
  if (DidFail(error))
    return;
  SetPicoJsonObjectValue(data, "physicalWidth", picojson::value(ret));

  ret = static_cast<double>(disp.GetPhysicalHeight(error));
  if (DidFail(error))
    return;
  SetPicoJsonObjectValue(data, "physicalHeight", picojson::value(ret));

  ret = static_cast<double>(disp.GetBrightness(error));
  if (DidFail(error))
    return;
  SetPicoJsonObjectValue(data, "brightness", picojson::value(ret));
}

void SystemInfoContext::GetBuild(picojson::value& error,
                                 picojson::value& data) {
  // FIXME(halton): Add actual implementation
  SetPicoJsonObjectValue(data, "model", picojson::value("Tizen PC"));
  SetPicoJsonObjectValue(data, "manufacturer", picojson::value("Intel Corp."));
  SetPicoJsonObjectValue(data, "buildVersion", picojson::value("3.0"));
  SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

void SystemInfoContext::GetLocale(picojson::value& error,
                                  picojson::value& data) {
  // FIXME(halton): Add actual implementation
  SetPicoJsonObjectValue(data, "language", picojson::value("zh_CN"));
  SetPicoJsonObjectValue(data, "country", picojson::value("PRC"));
  SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

void SystemInfoContext::HandleGetPropertyValue(const picojson::value& input,
                                               picojson::value& output) {
  picojson::value error;
  picojson::value data;
  std::string prop;

  error = picojson::value(picojson::object());
  data = picojson::value(picojson::object());

  SetPicoJsonObjectValue(error, "message", picojson::value(""));

  prop = input.get("prop").to_str();
  if (prop == "BATTERY") {
    GetBattery(error, data);
  } else if (prop == "CPU") {
    GetCPU(error, data);
  } else if (prop == "STORAGE") {
    GetStorage(error, data);
  } else if (prop == "DISPLAY") {
    GetDisplay(error, data);
  } else if (prop == "DEVICE_ORIENTATION ") {
    GetDeviceOrientation(error, data);
  } else if (prop == "BUILD") {
    GetBuild(error, data);
  } else if (prop == "LOCALE") {
    GetLocale(error, data);
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
}

void SystemInfoContext::HandleMessage(const char* message) {
  picojson::value input;
  picojson::value output;

  std::string err;
  picojson::parse(input, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }

  output = picojson::value(picojson::object());
  std::string reply_id = input.get("_reply_id").to_str();
  SetPicoJsonObjectValue(output, "_reply_id", picojson::value(reply_id));

  std::string cmd = input.get("cmd").to_str();
  if (cmd == "getPropertyValue")
    HandleGetPropertyValue(input, output);

  std::string result = output.serialize();
  api_->PostMessage(result.c_str());
}

void SystemInfoContext::HandleSyncMessage(const char*) {
}

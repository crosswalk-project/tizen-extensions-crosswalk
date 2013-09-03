// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_context.h"

#include <stdlib.h>
#if defined(TIZEN_MOBILE)
#include <system_info.h>
#endif

#include <string>

#include "common/picojson.h"
#include "system_info/system_info_utils.h"

CXWalkExtension* xwalk_extension_init(int32_t api_version) {
  return ExtensionAdapter<SystemInfoContext>::Initialize();
}

SystemInfoContext::SystemInfoContext(ContextAPI* api)
    : api_(api),
      battery_(SysInfoBattery::GetSysInfoBattery(api)),
      build_(SysInfoBuild::GetSysInfoBuild(api)),
      cellular_network_(
          SysInfoCellularNetwork::GetSysInfoCellularNetwork(api)),
      cpu_(SysInfoCpu::GetSysInfoCpu(api)),
      device_orientation_(
          SysInfoDeviceOrientation::GetSysInfoDeviceOrientation(api)),
      display_(SysInfoDisplay::GetSysInfoDisplay(api)),
      locale_(SysInfoLocale::GetSysInfoLocale(api)),
      sim_(SysInfoSim::GetSysInfoSim(api)),
      storage_(SysInfoStorage::GetSysInfoStorage(api)),
      network_(SysInfoNetwork::GetSysInfoNetwork(api)),
      peripheral_(SysInfoPeripheral::GetSysInfoPeripheral(api)),
      wifi_network_(SysInfoWifiNetwork::GetSysInfoWifiNetwork(api)) {
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
  system_info::SetPicoJsonObjectValue(output, "_reply_id",
      picojson::value(reply_id));

  picojson::value error = picojson::value(picojson::object());
  picojson::value data = picojson::value(picojson::object());

  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
  std::string prop = input.get("prop").to_str();

  if (prop == "BATTERY") {
    battery_.Get(error, data);
  } else if (prop == "CPU") {
    cpu_.Get(error, data);
  } else if (prop == "STORAGE") {
    storage_.Get(error, data);
  } else if (prop == "DISPLAY") {
    display_.Get(error, data);
  } else if (prop == "DEVICE_ORIENTATION") {
    device_orientation_.Get(error, data);
  } else if (prop == "BUILD") {
    build_.Get(error, data);
  } else if (prop == "LOCALE") {
    locale_.Get(error, data);
  } else if (prop == "NETWORK") {
    network_.Get(error, data);
  } else if (prop == "WIFI_NETWORK") {
    wifi_network_.Get(error, data);
  } else if (prop == "CELLULAR_NETWORK") {
    cellular_network_.Get(error, data);
  } else if (prop == "SIM") {
    sim_.Get(error, data);
  } else if (prop == "PERIPHERAL") {
    peripheral_.Get(error, data);
  } else {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Not supported property " + prop));
  }

  if (!error.get("message").to_str().empty()) {
    system_info::SetPicoJsonObjectValue(output, "error", error);
  } else {
    system_info::SetPicoJsonObjectValue(output, "data", data);
  }

  std::string result = output.serialize();
  api_->PostMessage(result.c_str());
}

void SystemInfoContext::HandleStartListening(const picojson::value& input) {
  std::string prop = input.get("prop").to_str();

  if (prop == "BATTERY") {
    battery_.StartListening();
  } else if (prop == "CPU") {
    cpu_.StartListening();
  } else if (prop == "STORAGE") {
    storage_.StartListening();
  } else if (prop == "DISPLAY") {
    display_.StartListening();
  } else if (prop == "DEVICE_ORIENTATION") {
    device_orientation_.StartListening();
  } else if (prop == "BUILD") {
    build_.StartListening();
  } else if (prop == "LOCALE") {
    locale_.StartListening();
  } else if (prop == "NETWORK") {
    // FIXME(halton): Add NETWORK listener
  } else if (prop == "WIFI_NETWORK") {
    // FIXME(halton): Add WIFI_NETWORK listener
  } else if (prop == "CELLULAR_NETWORK") {
    // FIXME(halton): Add CELLULAR_NETWORK listener
  } else if (prop == "SIM") {
    // FIXME(halton): Add SIM listener
  } else if (prop == "PERIPHERAL") {
    peripheral_.StartListening();
  }
}

void SystemInfoContext::HandleStopListening(const picojson::value& input) {
  std::string prop = input.get("prop").to_str();

  if (prop == "BATTERY") {
    battery_.StopListening();
  } else if (prop == "CPU") {
    cpu_.StopListening();
  } else if (prop == "STORAGE") {
    storage_.StopListening();
  } else if (prop == "DISPLAY") {
    display_.StopListening();
  } else if (prop == "DEVICE_ORIENTATION") {
    device_orientation_.StopListening();
  } else if (prop == "BUILD") {
    build_.StopListening();
  } else if (prop == "LOCALE") {
    locale_.StopListening();
  } else if (prop == "NETWORK") {
    network_.StopListening();
  } else if (prop == "WIFI_NETWORK") {
    wifi_network_.StopListening();
  } else if (prop == "CELLULAR_NETWORK") {
    cellular_network_.StopListening();
  } else if (prop == "SIM") {
    sim_.StopListening();
  } else if (prop == "PERIPHERAL") {
    peripheral_.StopListening();
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
  } else if (cmd == "startListening") {
    HandleStartListening(input);
  } else if (cmd == "stopListening") {
    HandleStopListening(input);
  }
}

void SystemInfoContext::HandleSyncMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "getCapabilities") {
    HandleGetCapabilities();
  } else {
    std::cout << "Not supported sync api " << cmd << "().\n";
  }
}

void SystemInfoContext::HandleGetCapabilities() {
  picojson::value::object o;

#if defined(TIZEN_MOBILE)
  bool b;
  int i;
  char* s;

  system_info_get_value_bool(SYSTEM_INFO_KEY_BLUETOOTH_SUPPORTED, &b);
  o["bluetooth"] = picojson::value(b);

  system_info_get_value_bool(SYSTEM_INFO_KEY_NFC_SUPPORTED, &b);
  o["nfc"] = picojson::value(b);

  system_info_get_value_bool(SYSTEM_INFO_KEY_NFC_RESERVED_PUSH_SUPPORTED, &b);
  o["nfcReservedPush"] = picojson::value(b);

  system_info_get_value_int(SYSTEM_INFO_KEY_MULTI_POINT_TOUCH_COUNT, &i);
  o["multiTouchCount"] = picojson::value(static_cast<double>(i));

  system_info_get_value_bool(SYSTEM_INFO_KEY_KEYBOARD_TYPE, &b);
  o["inputKeyboard"] = picojson::value(b);

  system_info_get_value_bool(SYSTEM_INFO_KEY_KEYBOARD_TYPE, &b);
  o["inputKeyboardLayout"] = picojson::value(b);

  system_info_get_value_bool(SYSTEM_INFO_KEY_WIFI_SUPPORTED, &b);
  o["wifi"] = picojson::value(b);

  system_info_get_value_bool(SYSTEM_INFO_KEY_WIFI_DIRECT_SUPPORTED, &b);
  o["wifiDirect"] = picojson::value(b);

  s = NULL;
  system_info_get_value_string(SYSTEM_INFO_KEY_OPENGLES_VERSION, &s);
  if (s && (strlen(s) != 0)) {
    o["opengles"] = picojson::value(true);

    if (strstr(s, "1.1"))
      o["openglesVersion1_1"] = picojson::value(true);
    else
      o["openglesVersion1_1"] = picojson::value(false);

    if (strstr(s, "2.0"))
      o["openglesVersion2_0"] = picojson::value(true);
    else
      o["openglesVersion2_0"] = picojson::value(false);
  } else {
    o["opengles"] = picojson::value(false);
    o["openglesVersion1_1"] = picojson::value(false);
    o["openglesVersion2_0"] = picojson::value(false);
  }

  s = NULL;
  system_info_get_value_string(SYSTEM_INFO_KEY_OPENGLES_TEXTURE_FORMAT, &s);
  SetStringPropertyValue(o, "openglestextureFormat", s);

  system_info_get_value_bool(SYSTEM_INFO_KEY_FMRADIO_SUPPORTED, &b);
  o["fmRadio"] = picojson::value(b);

  s = NULL;
  system_info_get_value_string(SYSTEM_INFO_KEY_TIZEN_VERSION_NAME, &s);
  SetStringPropertyValue(o, "platformVersion", s);

  s = NULL;
  system_info_get_value_string(SYSTEM_INFO_KEY_TIZEN_VERSION, &s);
  SetStringPropertyValue(o, "webApiVersion", s);

  // FIXME(halton): find which key reflect this prop
  o["nativeApiVersion"] = picojson::value("Unknown");

  s = NULL;
  system_info_get_value_string(SYSTEM_INFO_KEY_PLATFORM_NAME, &s);
  SetStringPropertyValue(o, "platformName", s);

  system_info_get_value_int(SYSTEM_INFO_KEY_CAMERA_COUNT, &i);
  o["camera"] = picojson::value(i > 0);

  system_info_get_value_bool(SYSTEM_INFO_KEY_FRONT_CAMERA_SUPPORTED, &b);
  o["cameraFront"] = picojson::value(b);

  system_info_get_value_bool(SYSTEM_INFO_KEY_FRONT_CAMERA_FLASH_SUPPORTED, &b);
  o["cameraFrontFlash"] = picojson::value(b);

  system_info_get_value_bool(SYSTEM_INFO_KEY_BACK_CAMERA_SUPPORTED, &b);
  o["cameraBack"] = picojson::value(b);

  system_info_get_value_bool(SYSTEM_INFO_KEY_BACK_CAMERA_FLASH_SUPPORTED, &b);
  o["cameraBackFlash"] = picojson::value(b);

  bool b_gps;
  system_info_get_value_bool(SYSTEM_INFO_KEY_GPS_SUPPORTED, &b_gps);
  o["locationGps"] = picojson::value(b_gps);

  system_info_get_value_bool(SYSTEM_INFO_KEY_WPS_SUPPORTED, &b);
  o["locationWps"] = picojson::value(b);

  o["location"] = picojson::value(b && b_gps);

  system_info_get_value_bool(SYSTEM_INFO_KEY_MICROPHONE_SUPPORTED, &b);
  o["microphone"] = picojson::value(b);

  system_info_get_value_bool(SYSTEM_INFO_KEY_USB_HOST_SUPPORTED, &b);
  o["usbHost"] = picojson::value(b);

  system_info_get_value_bool(SYSTEM_INFO_KEY_USB_ACCESSORY_SUPPORTED, &b);
  o["usbAccessory"] = picojson::value(b);

  system_info_get_value_bool(SYSTEM_INFO_KEY_RCA_SUPPORTED, &b);
  o["screenOutputRca"] = picojson::value(b);

  system_info_get_value_bool(SYSTEM_INFO_KEY_HDMI_SUPPORTED, &b);
  o["screenOutputHdmi"] = picojson::value(b);

  s = NULL;
  system_info_get_value_string(SYSTEM_INFO_KEY_CORE_CPU_ARCH, &s);
  SetStringPropertyValue(o, "platformCoreCpuArch", s);

  s = NULL;
  system_info_get_value_string(SYSTEM_INFO_KEY_CORE_FPU_ARCH, &s);
  SetStringPropertyValue(o, "platformCoreFpuArch", s);

  system_info_get_value_bool(SYSTEM_INFO_KEY_SIP_VOIP_SUPPORTED, &b);
  o["sipVoip"] = picojson::value(b);

  s = NULL;
  system_info_get_value_string(SYSTEM_INFO_KEY_DEVICE_UUID, &s);
  SetStringPropertyValue(o, "duid", s);

  system_info_get_value_bool(SYSTEM_INFO_KEY_SPEECH_RECOGNITION_SUPPORTED, &b);
  o["speechRecognition"] = picojson::value(b);

  // FIXME(halton): find which key reflect this prop
  o["speechSynthesis"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["accelerometer"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["accelerometerWakeup"] = picojson::value(false);

  system_info_get_value_bool(SYSTEM_INFO_KEY_BAROMETER_SENSOR_SUPPORTED, &b);
  o["barometer"] = picojson::value(b);

  // FIXME(halton): find which key reflect this prop
  o["barometerWakeup"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["gyroscope"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["gyroscopeWakeup"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["magnetometer"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["magnetometerWakeup"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["photometer"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["photometerWakeup"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["proximity"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["proximityWakeup"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["tiltmeter"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["tiltmeterWakeup"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["dataEncryption"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["graphicsAcceleration"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["push"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["telephony"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["telephonyMms"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["telephonySms"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["screenSizeNormal"] = picojson::value(false);

  int height;
  int width;
  system_info_get_value_int(SYSTEM_INFO_KEY_SCREEN_HEIGHT, &height);
  system_info_get_value_int(SYSTEM_INFO_KEY_SCREEN_WIDTH, &width);
  o["screenSize480_800"] = picojson::value(false);
  o["screenSize720_1280"] = picojson::value(false);
  if ((width == 480) && (height == 800)) {
    o["screenSize480_800"] = picojson::value(true);
  } else if ((width == 720) && (height == 1280)) {
    o["screenSize720_1280"] = picojson::value(true);
  }

  // FIXME(halton): find which key reflect this prop
  o["autoRotation"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["shellAppWidget"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["visionImageRecognition"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["visionQrcodeGeneration"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["visionQrcodeRecognition"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["visionFaceRecognition"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["secureElement"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["nativeOspCompatible"] = picojson::value(false);

  // FIXME(halton): find which key reflect this prop
  o["profile"] = picojson::value("MOBILE_WEB");

  o["error"] = picojson::value("");
#elif defined(GENERIC_DESKTOP)
  o["error"] = picojson::value("getCapabilities is not supported on desktop.");
#endif

  picojson::value v(o);
  api_->SetSyncReply(v.serialize().c_str());
}

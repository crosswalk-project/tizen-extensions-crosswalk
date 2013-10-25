// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_instance.h"

#include <stdlib.h>
#if defined(TIZEN_MOBILE)
#include <pkgmgr-info.h>
#include <sensors.h>
#include <system_info.h>
#endif

#include <string>
#include <utility>

#include "common/picojson.h"
#include "system_info/system_info_battery.h"
#include "system_info/system_info_build.h"
#include "system_info/system_info_cellular_network.h"
#include "system_info/system_info_cpu.h"
#include "system_info/system_info_device_orientation.h"
#include "system_info/system_info_display.h"
#include "system_info/system_info_locale.h"
#include "system_info/system_info_network.h"
#include "system_info/system_info_peripheral.h"
#include "system_info/system_info_sim.h"
#include "system_info/system_info_storage.h"
#include "system_info/system_info_utils.h"
#include "system_info/system_info_wifi_network.h"

const char* sSystemInfoFilePath = "/usr/etc/system-info.ini";

template <class T>
void SystemInfoInstance::RegisterClass() {
  classes_.insert(SysInfoClassPair(T::name_ , T::GetInstance()));
}

SystemInfoInstance::~SystemInfoInstance() {
  for (classes_iterator it = classes_.begin();
       it != classes_.end(); ++it) {
    (it->second).RemoveListener(this);
  }
}

void SystemInfoInstance::InstancesMapInitialize() {
  RegisterClass<SysInfoBattery>();
  RegisterClass<SysInfoBuild>();
  RegisterClass<SysInfoCellularNetwork>();
  RegisterClass<SysInfoCpu>();
  RegisterClass<SysInfoDeviceOrientation>();
  RegisterClass<SysInfoDisplay>();
  RegisterClass<SysInfoLocale>();
  RegisterClass<SysInfoNetwork>();
  RegisterClass<SysInfoPeripheral>();
  RegisterClass<SysInfoSim>();
  RegisterClass<SysInfoStorage>();
  RegisterClass<SysInfoWifiNetwork>();
}

void SystemInfoInstance::HandleGetPropertyValue(const picojson::value& input,
                                               picojson::value& output) {
  std::string reply_id = input.get("_reply_id").to_str();
  system_info::SetPicoJsonObjectValue(output, "_reply_id",
      picojson::value(reply_id));

  picojson::value error = picojson::value(picojson::object());
  picojson::value data = picojson::value(picojson::object());

  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
  std::string prop = input.get("prop").to_str();
  classes_iterator it= classes_.find(prop);

  if (it == classes_.end()) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Property not supported: " + prop));
  } else {
    (it->second).Get(error, data);
  }

  if (!error.get("message").to_str().empty()) {
    system_info::SetPicoJsonObjectValue(output, "error", error);
  } else {
    system_info::SetPicoJsonObjectValue(output, "data", data);
  }

  std::string result = output.serialize();
  PostMessage(result.c_str());
}

void SystemInfoInstance::HandleStartListening(const picojson::value& input) {
  std::string prop = input.get("prop").to_str();
  classes_iterator it= classes_.find(prop);

  if (it != classes_.end()) {
    (it->second).AddListener(this);
  }
}

void SystemInfoInstance::HandleStopListening(const picojson::value& input) {
  std::string prop = input.get("prop").to_str();
  classes_iterator it= classes_.find(prop);

  if (it != classes_.end()) {
    (it->second).RemoveListener(this);
  }
}

void SystemInfoInstance::HandleMessage(const char* message) {
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

void SystemInfoInstance::HandleSyncMessage(const char* message) {
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

void SystemInfoInstance::HandleGetCapabilities() {
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
  free(s);

  s = NULL;
  system_info_get_value_string(SYSTEM_INFO_KEY_OPENGLES_TEXTURE_FORMAT, &s);
  SetStringPropertyValue(o, "openglestextureFormat", s ? s : "");
  free(s);

  system_info_get_value_bool(SYSTEM_INFO_KEY_FMRADIO_SUPPORTED, &b);
  o["fmRadio"] = picojson::value(b);

  s = NULL;
  system_info_get_value_string(SYSTEM_INFO_KEY_TIZEN_VERSION, &s);
  SetStringPropertyValue(o, "platformVersion", s ? s : "");
  free(s);

  std::string version =
      system_info::GetPropertyFromFile(
          sSystemInfoFilePath,
          "http://tizen.org/feature/platform.web.api.version");
  SetStringPropertyValue(o, "webApiVersion", version.c_str());

  version = system_info::GetPropertyFromFile(
                sSystemInfoFilePath,
                "http://tizen.org/feature/platform.native.api.version");
  SetStringPropertyValue(o, "nativeApiVersion", version.c_str());

  s = NULL;
  system_info_get_value_string(SYSTEM_INFO_KEY_PLATFORM_NAME, &s);
  SetStringPropertyValue(o, "platformName", s ? s : "");
  free(s);

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
  SetStringPropertyValue(o, "platformCoreCpuArch", s ? s : "");
  free(s);

  s = NULL;
  system_info_get_value_string(SYSTEM_INFO_KEY_CORE_FPU_ARCH, &s);
  SetStringPropertyValue(o, "platformCoreFpuArch", s ? s : "");
  free(s);

  system_info_get_value_bool(SYSTEM_INFO_KEY_SIP_VOIP_SUPPORTED, &b);
  o["sipVoip"] = picojson::value(b);

  s = NULL;
  system_info_get_value_string(SYSTEM_INFO_KEY_DEVICE_UUID, &s);
  SetStringPropertyValue(o, "duid", s ? s : "");
  free(s);

  system_info_get_value_bool(SYSTEM_INFO_KEY_SPEECH_RECOGNITION_SUPPORTED, &b);
  o["speechRecognition"] = picojson::value(b);

  b = system_info::PathExists("/usr/lib/libtts.so");
  o["speechSynthesis"] = picojson::value(b);

  sensor_is_supported(SENSOR_ACCELEROMETER, &b);
  o["accelerometer"] = picojson::value(b);

  sensor_awake_is_supported(SENSOR_ACCELEROMETER, &b);
  o["accelerometerWakeup"] = picojson::value(b);

  system_info_get_value_bool(SYSTEM_INFO_KEY_BAROMETER_SENSOR_SUPPORTED, &b);
  o["barometer"] = picojson::value(b);

  // FIXME(halton): find which key reflect this prop
  o["barometerWakeup"] = picojson::value(false);

  sensor_is_supported(SENSOR_GYROSCOPE, &b);
  o["gyroscope"] = picojson::value(b);

  sensor_awake_is_supported(SENSOR_GYROSCOPE, &b);
  o["gyroscopeWakeup"] = picojson::value(b);

  sensor_is_supported(SENSOR_MAGNETIC, &b);
  o["magnetometer"] = picojson::value(b);

  sensor_awake_is_supported(SENSOR_MAGNETIC, &b);
  o["magnetometerWakeup"] = picojson::value(b);

  sensor_is_supported(SENSOR_LIGHT, &b);
  o["photometer"] = picojson::value(b);

  sensor_awake_is_supported(SENSOR_LIGHT, &b);
  o["photometerWakeup"] = picojson::value(b);

  sensor_is_supported(SENSOR_PROXIMITY, &b);
  o["proximity"] = picojson::value(b);

  sensor_awake_is_supported(SENSOR_PROXIMITY, &b);
  o["proximityWakeup"] = picojson::value(b);

  sensor_is_supported(SENSOR_MOTION_TILT, &b);
  o["tiltmeter"] = picojson::value(b);

  sensor_awake_is_supported(SENSOR_MOTION_TILT, &b);
  o["tiltmeterWakeup"] = picojson::value(b);

  b = system_info::PathExists("/usr/lib/libsqlite3.so.0");
  o["dataEncryption"] = picojson::value(b);

  system_info_get_value_bool(SYSTEM_INFO_KEY_GRAPHICS_HWACCEL_SUPPORTED, &b);
  o["graphicsAcceleration"] = picojson::value(b);

  b = system_info::PathExists("/usr/bin/pushd");
  o["push"] = picojson::value(b);

  b = system_info::PathExists("/usr/bin/telephony-daemon");
  o["telephony"] = picojson::value(b);

  system_info_get_value_bool(SYSTEM_INFO_KEY_MMS_SUPPORTED, &b);
  o["telephonyMms"] = picojson::value(b);

  system_info_get_value_bool(SYSTEM_INFO_KEY_SMS_SUPPORTED, &b);
  o["telephonySms"] = picojson::value(b);

  std::string screensize_normal =
      system_info::GetPropertyFromFile(
          sSystemInfoFilePath,
          "http://tizen.org/feature/screen.coordinate_system.size.normal");
  o["screenSizeNormal"] =
      picojson::value(system_info::ParseBoolean(screensize_normal));

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

  system_info_get_value_bool(SYSTEM_INFO_KEY_FEATURE_AUTO_ROTATION_SUPPORTED,
                             &b);
  o["autoRotation"] = picojson::value(b);

  pkgmgrinfo_pkginfo_h handle;
  if (pkgmgrinfo_pkginfo_get_pkginfo("gi2qxenosh", &handle) == PMINFO_R_OK)
    o["shellAppWidget"] = picojson::value(true);
  else
    o["shellAppWidget"] = picojson::value(false);

  b = system_info::PathExists("/usr/lib/osp/libarengine.so");
  o["visionImageRecognition"] = picojson::value(b);
  o["visionQrcodeGeneration"] = picojson::value(b);
  o["visionQrcodeRecognition"] = picojson::value(b);
  o["visionFaceRecognition"] = picojson::value(b);

  b = system_info::PathExists("/usr/bin/smartcard-daemon");
  o["secureElement"] = picojson::value(b);

  std::string osp_compatible =
      system_info::GetPropertyFromFile(
          sSystemInfoFilePath,
          "http://tizen.org/feature/platform.native.osp_compatible");
  o["nativeOspCompatible"] =
      picojson::value(system_info::ParseBoolean(osp_compatible));

  // FIXME(halton): Not supported until Tizen 2.2
  o["profile"] = picojson::value("MOBILE_WEB");

  o["error"] = picojson::value("");
#elif defined(GENERIC_DESKTOP)
  o["error"] = picojson::value("getCapabilities is not supported on desktop.");
#endif

  picojson::value v(o);
  SendSyncReply(v.serialize().c_str());
}

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Copyright (c) 2012 Samsung Electronics Co., Ltd.

// Use of this source code is governed by a BSD-style license
// AND  Apache License, Version 2.0  that can be
// found in the LICENSE & LICENSE.AL2 files.

// you may not use this file except in compliance with theses Licenses.
// You may obtain a copy of the APACHE License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "system_info/system_info_instance.h"

#include <dlfcn.h>
#include <stdlib.h>
#if defined(TIZEN)
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
#if defined(TIZEN)
#include "system_info/system_info_sim.h"
#endif
#include "system_info/system_info_storage.h"
#include "system_info/system_info_utils.h"
#include "system_info/system_info_wifi_network.h"

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
#if defined(TIZEN)
  RegisterClass<SysInfoSim>();
#endif
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

#if defined(TIZEN)
  bool b;
  int i;
  char* s = NULL;
  std::string string_full;

  if (system_info_get_platform_bool("tizen.org/feature/network.bluetooth",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["bluetooth"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/network.nfc",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["nfc"] = picojson::value(b);

  if (system_info_get_platform_bool(
      "tizen.org/feature/network.nfc.reserved_push",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["nfcReservedPush"] = picojson::value(b);

  if (system_info_get_platform_int(
      "tizen.org/feature/multi_point_touch.point_count",
      &i) == SYSTEM_INFO_ERROR_NONE)
    o["multiTouchCount"] = picojson::value(static_cast<double>(i));

  if (system_info_get_platform_bool("tizen.org/feature/input.keyboard",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["inputKeyboard"] = picojson::value(b);

  if (system_info_get_platform_string(
      "tizen.org/feature/input.keyboard.layout",
      &s) == SYSTEM_INFO_ERROR_NONE) {
    o["inputKeyboardLayout"] = picojson::value(s);
    free(s);
  }

  if (system_info_get_platform_bool("tizen.org/feature/network.wifi",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["wifi"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/network.wifi.direct",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["wifiDirect"] = picojson::value(b);

  if ((system_info_get_platform_bool("tizen.org/feature/opengles",
      &b) == SYSTEM_INFO_ERROR_NONE)
      && b == true) {
    o["opengles"] = picojson::value(true);
    if (system_info_get_platform_bool("tizen.org/feature/opengles.version.1_1",
      &b) == SYSTEM_INFO_ERROR_NONE)
      o["openglesVersion1_1"] = picojson::value(b);

    if (system_info_get_platform_bool("tizen.org/feature/opengles.version.2_0",
      &b) == SYSTEM_INFO_ERROR_NONE)
      o["openglesVersion2_0"] = picojson::value(b);
  } else {
    o["opengles"] = picojson::value(false);
    o["openglesVersion1_1"] = picojson::value(false);
    o["openglesVersion2_0"] = picojson::value(false);
  }

  if (system_info_get_platform_bool(
      "tizen.org/feature/opengles.texture_format.utc",
      &b) == SYSTEM_INFO_ERROR_NONE
      && b == true) {
    string_full += "utc";
  }
  if (system_info_get_platform_bool(
      "tizen.org/feature/opengles.texture_format.ptc",
      &b) == SYSTEM_INFO_ERROR_NONE
      && b == true) {
    if (!string_full.empty())
      string_full += " | ";
    string_full +=  "ptc";
  }
  if (system_info_get_platform_bool(
      "tizen.org/feature/opengles.texture_format.etc",
      &b) == SYSTEM_INFO_ERROR_NONE
      && b == true) {
    if (!string_full.empty())
      string_full +=  " | ";
    string_full += "etc";
  }
  if (system_info_get_platform_bool(
      "tizen.org/feature/opengles.texture_format.3dc",
      &b) == SYSTEM_INFO_ERROR_NONE
      && b == true) {
    if (!string_full.empty())
      string_full +=  " | ";
    string_full += "3dc";
  }
  if (system_info_get_platform_bool(
      "tizen.org/feature/opengles.texture_format.atc",
      &b) == SYSTEM_INFO_ERROR_NONE
      && b == true) {
    if (!string_full.empty())
      string_full +=  " | ";
    string_full += "atc";
  }
  if (system_info_get_platform_bool(
      "tizen.org/feature/opengles.texture_format.pvrtc",
      &b) == SYSTEM_INFO_ERROR_NONE
      && b == true) {
    if (!string_full.empty())
      string_full +=  " | ";
    string_full +=  "pvrtc";
  }

  SetStringPropertyValue(o, "openglestextureFormat",
                         string_full.c_str() ? string_full.c_str() : "");
  string_full.clear();

  if (system_info_get_platform_bool("tizen.org/feature/fmradio",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["fmRadio"] = picojson::value(b);

  if (system_info_get_platform_string("tizen.org/feature/platform.version",
      &s) == SYSTEM_INFO_ERROR_NONE) {
    SetStringPropertyValue(o, "platformVersion", s ? s : "");
    free(s);
  }

  if (system_info_get_platform_string(
      "tizen.org/feature/platform.web.api.version",
      &s) == SYSTEM_INFO_ERROR_NONE) {
    SetStringPropertyValue(o, "webApiVersion", s);
    free(s);
  }
  if (system_info_get_platform_string(
      "tizen.org/feature/platform.native.api.version",
      &s) == SYSTEM_INFO_ERROR_NONE) {
    SetStringPropertyValue(o, "nativeApiVersion", s);
    free(s);
  }
  if (system_info_get_platform_string("tizen.org/system/platform.name",
      &s) == SYSTEM_INFO_ERROR_NONE) {
    SetStringPropertyValue(o, "platformName", s ? s : "");
    free(s);
  }

  if (system_info_get_platform_bool("tizen.org/feature/camera",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["camera"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/camera.front",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["cameraFront"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/camera.front.flash",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["cameraFrontFlash"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/camera.back",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["cameraBack"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/camera.back.flash",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["cameraBackFlash"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/location",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["location"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/location.gps",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["locationGps"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/location.wps",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["locationWps"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/microphone",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["microphone"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/usb.host",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["usbHost"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/usb.accessory",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["usbAccessory"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/screen.output.rca",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["screenOutputRca"] = picojson::value(b);

  if (system_info_get_platform_bool(
      "tizen.org/feature/screen.output.hdmi",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["screenOutputHdmi"] = picojson::value(b);

  if (system_info_get_platform_bool(
      "tizen.org/feature/platform.core.cpu.arch.armv6",
      &b) == SYSTEM_INFO_ERROR_NONE
      && b == true) {
    string_full += "armv6";
  }
  if (system_info_get_platform_bool(
      "tizen.org/feature/platform.core.cpu.arch.armv7",
      &b) == SYSTEM_INFO_ERROR_NONE
      && b == true) {
    if (!string_full.empty())
      string_full +=  " | ";
    string_full += "armv7";
  }
  if (system_info_get_platform_bool(
      "tizen.org/feature/platform.core.cpu.arch.x86",
      &b) == SYSTEM_INFO_ERROR_NONE
      && b == true) {
    if (!string_full.empty())
      string_full +=  " | ";
    string_full += "x86";
  }

  SetStringPropertyValue(o, "platformCoreCpuArch",
                         string_full.c_str() ? string_full.c_str() : "");
  string_full.clear();

  if (system_info_get_platform_bool(
      "tizen.org/feature/platform.core.fpu.arch.sse2",
      &b) == SYSTEM_INFO_ERROR_NONE
      && b == true) {
    string_full += "sse2";
  }
  if (system_info_get_platform_bool(
      "tizen.org/feature/platform.core.fpu.arch.sse3",
      &b) == SYSTEM_INFO_ERROR_NONE
      && b == true) {
    if (!string_full.empty())
      string_full +=  " | ";
    string_full += "sse3";
  }
  if (system_info_get_platform_bool(
      "tizen.org/feature/platform.core.fpu.arch.ssse3",
      &b) == SYSTEM_INFO_ERROR_NONE
      && b == true) {
    if (!string_full.empty())
      string_full +=  " | ";
    string_full += "ssse3";
  }
  if (system_info_get_platform_bool(
      "tizen.org/feature/platform.core.fpu.arch.vfpv2",
      &b) == SYSTEM_INFO_ERROR_NONE
      && b == true) {
    if (!string_full.empty())
      string_full +=  " | ";
    string_full += "vfpv2";
  }
  if (system_info_get_platform_bool(
      "tizen.org/feature/platform.core.fpu.arch.vfpv3",
      &b) == SYSTEM_INFO_ERROR_NONE
      && b == true) {
    if (!string_full.empty())
      string_full +=  " | ";
    string_full += "vfpv3";
  }
  SetStringPropertyValue(o, "platformCoreFpuArch",
                         string_full.c_str() ? string_full.c_str() : "");
  string_full.clear();

  if (system_info_get_platform_bool("tizen.org/feature/sip.voip",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["sipVoip"] = picojson::value(b);

  s = system_info::GetDuidProperty();
  SetStringPropertyValue(o, "duid", s ? s : "");
  free(s);

  if (system_info_get_platform_bool(
      "tizen.org/feature/speech.recognition",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["speechRecognition"] = picojson::value(b);

  if (system_info_get_platform_bool(
     "tizen.org/feature/speech.synthesis",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["speechSynthesis"] = picojson::value(b);

  if (system_info_get_platform_bool(
      "tizen.org/feature/sensor.accelerometer",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["accelerometer"] = picojson::value(b);

  if (system_info_get_platform_bool(
      "tizen.org/feature/sensor.accelerometer.wakeup",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["accelerometerWakeup"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/sensor.barometer",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["barometer"] = picojson::value(b);

  if (system_info_get_platform_bool(
      "tizen.org/feature/sensor.barometer.wakeup",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["barometerWakeup"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/sensor.gyroscope",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["gyroscope"] = picojson::value(b);

  if (system_info_get_platform_bool(
      "tizen.org/feature/sensor.gyroscope.wakeup",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["gyroscopeWakeup"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/sensor.magnetometer",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["magnetometer"] = picojson::value(b);

  if (system_info_get_platform_bool(
      "tizen.org/feature/sensor.magnetometer.wakeup",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["magnetometerWakeup"] = picojson::value(b);

  if (system_info_get_platform_bool(
      "tizen.org/feature/sensor.photometer",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["photometer"] = picojson::value(b);

  if (system_info_get_platform_bool(
     "tizen.org/feature/sensor.photometer.wakeup",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["photometerWakeup"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/sensor.proximity",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["proximity"] = picojson::value(b);

  if (system_info_get_platform_bool(
      "tizen.org/feature/sensor.proximity.wakeup",
      &b) == SYSTEM_INFO_ERROR_NONE)
  o["proximityWakeup"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/sensor.tiltmeter",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["tiltmeter"] = picojson::value(b);

  if (system_info_get_platform_bool(
      "tizen.org/feature/sensor.tiltmeter.wakeup",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["tiltmeterWakeup"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/database.encryption",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["dataEncryption"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/graphics.acceleration",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["graphicsAcceleration"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/network.push",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["push"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/network.telephony",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["telephony"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/network.telephony.mms",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["telephonyMms"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/network.telephony.sms",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["telephonySms"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/screen.size.normal",
      &b) == SYSTEM_INFO_ERROR_NONE)
  o["screenSizeNormal"] =  picojson::value(b);

  if (system_info_get_platform_bool(
      "tizen.org/feature/screen.size.normal.480.800",
      &b) == SYSTEM_INFO_ERROR_NONE)
      o["screenSize480_800"] = picojson::value(b);

  if (system_info_get_platform_bool(
      "tizen.org/feature/screen.size.normal.720.1280",
      &b) == SYSTEM_INFO_ERROR_NONE)
      o["screenSize720_1280"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/screen.auto_rotation",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["autoRotation"] = picojson::value(b);

  if (system_info_get_platform_bool("tizen.org/feature/shell.appwidget",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["shellAppWidget"] = picojson::value(b);

  if (system_info_get_platform_bool(
      "tizen.org/feature/vision.image_recognition",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["visionImageRecognition"] = picojson::value(b);
  if (system_info_get_platform_bool(
      "tizen.org/feature/vision.qrcode_generation",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["visionQrcodeGeneration"] = picojson::value(b);
  if (system_info_get_platform_bool(
      "tizen.org/feature/vision.qrcode_recognition",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["visionQrcodeRecognition"] = picojson::value(b);
  if (system_info_get_platform_bool(
      "tizen.org/feature/vision.face_recognition",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["visionFaceRecognition"] = picojson::value(b);

  if (system_info_get_platform_bool(
      "tizen.org/feature/network.secure_element",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["secureElement"] = picojson::value(b);

  if (system_info_get_platform_bool(
      "tizen.org/feature/platform.native.osp_compatible",
      &b) == SYSTEM_INFO_ERROR_NONE)
    o["nativeOspCompatible"] = picojson::value(b);

  if (system_info_get_platform_string("tizen.org/feature/profile",
      &s)  == SYSTEM_INFO_ERROR_NONE) {
    o["profile"] = picojson::value(s ? s : "");
    free(s);
  }

  o["error"] = picojson::value("");
#elif defined(GENERIC_DESKTOP)
  o["error"] = picojson::value("getCapabilities is not supported on desktop.");
#endif

  picojson::value v(o);
  SendSyncReply(v.serialize().c_str());
}

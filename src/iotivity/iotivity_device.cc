/*
 * Copyright (c) 2015 Cisco and/or its affiliates. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Cisco nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "iotivity/iotivity_device.h"
#include "common/extension.h"
#include "iotivity/iotivity_server.h"
#include "iotivity/iotivity_client.h"

IotivityDeviceInfo::IotivityDeviceInfo() {}

IotivityDeviceInfo::~IotivityDeviceInfo() {}

std::string IotivityDeviceInfo::hasMap(std::string key) {
  std::map<std::string, std::string>::const_iterator it;
  if ((it = m_deviceinfomap.find(key)) != m_deviceinfomap.end()) {
    DEBUG_MSG("Found %s=%s\n", key.c_str(), m_deviceinfomap[key].c_str());
    return m_deviceinfomap[key];
  }
  return "";
}

int IotivityDeviceInfo::mapSize() { return m_deviceinfomap.size(); }

void IotivityDeviceInfo::deserialize(const picojson::value& value) {
  DEBUG_MSG("IotivityDeviceInfo::deserialize\n");
  picojson::value properties = value.get("info");
  picojson::object& propertiesobject = properties.get<picojson::object>();
  DEBUG_MSG("value: size=%d\n", propertiesobject.size());

  m_deviceinfomap.clear();
  for (picojson::value::object::iterator iter = propertiesobject.begin();
       iter != propertiesobject.end(); ++iter) {
    std::string objectKey = iter->first;
    picojson::value objectValue = iter->second;

    if (objectValue.is<string>()) {
      DEBUG_MSG("[string] key=%s, value=%s\n", objectKey.c_str(),
                objectValue.get<string>().c_str());
      m_deviceinfomap[objectKey] = objectValue.get<string>();
    }
  }
}

void IotivityDeviceInfo::serialize(picojson::object& object) {
  for (auto const& entity : m_deviceinfomap) {
    object[entity.first] = picojson::value(entity.second);
  }
}

IotivityDeviceSettings::IotivityDeviceSettings() {
  m_url = "0.0.0.0:0";
  // m_deviceInfo;
  m_role = "intermediate";
  m_connectionMode = "acked";
}

IotivityDeviceSettings::~IotivityDeviceSettings() {}

void IotivityDeviceSettings::deserialize(const picojson::value& value) {
  DEBUG_MSG("IotivityDeviceSettings::deserialize\n");
  if (value.contains("url")) {
    m_url = value.get("url").to_str();
  }
  if (value.contains("role")) {
    m_role = value.get("role").to_str();
  }
  if (value.contains("connectionMode")) {
    m_connectionMode = value.get("connectionMode").to_str();
  }
  if (value.contains("info")) {
    m_deviceInfo.deserialize(value);
  }
}

IotivityDevice::IotivityDevice(common::Instance* instance)
    : m_instance(instance) {
  m_server = new IotivityServer(this);
  m_client = new IotivityClient(this);
}

IotivityDevice::IotivityDevice(common::Instance* instance,
                               IotivityDeviceSettings* settings) {
  m_instance = instance;
  m_server = new IotivityServer(this);
  m_client = new IotivityClient(this);

  if (settings == NULL) {
    // Set default
    configure(NULL);
  } else {
    configure(settings);
    configurePlatformInfo(settings->m_deviceInfo);
    configureDeviceInfo(settings->m_deviceInfo);
  }
}

IotivityDevice::~IotivityDevice() {
  delete m_server;
  delete m_client;
}

common::Instance* IotivityDevice::getInstance() { return m_instance; }

IotivityServer* IotivityDevice::getServer() { return m_server; }

IotivityClient* IotivityDevice::getClient() { return m_client; }

static void DuplicateString(char** targetString, std::string sourceString) {
  *targetString = new char[sourceString.length() + 1];
  strncpy(*targetString, sourceString.c_str(), (sourceString.length() + 1));
}

void DeletePlatformInfo(OCPlatformInfo& platformInfo) {
  delete[] platformInfo.platformID;
  delete[] platformInfo.manufacturerName;
  delete[] platformInfo.manufacturerUrl;
  delete[] platformInfo.modelNumber;
  delete[] platformInfo.dateOfManufacture;
  delete[] platformInfo.platformVersion;
  delete[] platformInfo.operatingSystemVersion;
  delete[] platformInfo.hardwareVersion;
  delete[] platformInfo.firmwareVersion;
  delete[] platformInfo.supportUrl;
  delete[] platformInfo.systemTime;
}

void DeleteDeviceInfo(OCDeviceInfo& deviceInfo) {
  delete[] deviceInfo.deviceName;
}

static OCStackResult SetPlatformInfo(
    OCPlatformInfo& platformInfo, std::string platformID,
    std::string manufacturerName, std::string manufacturerUrl,
    std::string modelNumber, std::string dateOfManufacture,
    std::string platformVersion, std::string operatingSystemVersion,
    std::string hardwareVersion, std::string firmwareVersion,
    std::string supportUrl, std::string systemTime) {
  DuplicateString(&platformInfo.platformID, platformID);
  DuplicateString(&platformInfo.manufacturerName, manufacturerName);
  DuplicateString(&platformInfo.manufacturerUrl, manufacturerUrl);
  DuplicateString(&platformInfo.modelNumber, modelNumber);
  DuplicateString(&platformInfo.dateOfManufacture, dateOfManufacture);
  DuplicateString(&platformInfo.platformVersion, platformVersion);
  DuplicateString(&platformInfo.operatingSystemVersion, operatingSystemVersion);
  DuplicateString(&platformInfo.hardwareVersion, hardwareVersion);
  DuplicateString(&platformInfo.firmwareVersion, firmwareVersion);
  DuplicateString(&platformInfo.supportUrl, supportUrl);
  DuplicateString(&platformInfo.systemTime, systemTime);
  return OC_STACK_OK;
}

static OCStackResult SetDeviceInfo(OCDeviceInfo& deviceInfo, std::string name) {
  DuplicateString(&deviceInfo.deviceName, name);
}

void IotivityDevice::configure(IotivityDeviceSettings* settings) {
  OC::QualityOfService QoS = OC::QualityOfService::LowQos;
  OC::ModeType modeType = OC::ModeType::Both;
  std::string host = "0.0.0.0";
  int port = 0;

  if (settings) {
    if (settings->m_connectionMode == "non-acked") {
      QoS = OC::QualityOfService::HighQos;
    } else {
      QoS = OC::QualityOfService::LowQos;
    }

    if (settings->m_role == "server") {
      modeType = OC::ModeType::Server;
    } else if (settings->m_role == "client") {
      modeType = OC::ModeType::Client;
    } else {
      modeType = OC::ModeType::Both;
    }

    if (settings->m_url != "") {
      std::string tmp = settings->m_url;
      std::string delimiter = ":";
      size_t pos = tmp.find(delimiter);
      if (pos != std::string::npos) {
        host = tmp.substr(0, pos);
        std::string portString = tmp.substr(pos + 1, tmp.length());
        DEBUG_MSG("host=%s, portString=%s\n", host.c_str(), portString.c_str());
        port = atoi(portString.c_str());
      }
    }
  } else {
    // Apply default
  }

  // By setting to "0.0.0.0", it binds to all available interfaces
  // Uses randomly available port
  PlatformConfig cfg{ServiceType::InProc, modeType, host.c_str(), 0, QoS};

  DEBUG_MSG("OCPlatform::Configure: host=%s:%d\n", host.c_str(), port);

  DEBUG_MSG("modeType=%d, QoS=%d\n", modeType, QoS);

  OCPlatform::Configure(cfg);
}

OCStackResult IotivityDevice::configurePlatformInfo(
    IotivityDeviceInfo& deviceInfo) {
  OCStackResult result = OC_STACK_ERROR;
  OCPlatformInfo platformInfo = {0};

  DEBUG_MSG("configurePlatformInfo %d\n", deviceInfo.mapSize());

  if (deviceInfo.mapSize() == 0) {
    // nothing to do
    return OC_STACK_OK;
  }

  std::string platformID = deviceInfo.hasMap("uuid");
  // name
  std::string hardwareVersion = deviceInfo.hasMap("coreSpecVersion");
  std::string operatingSystemVersion = deviceInfo.hasMap("osVersion");
  std::string modelNumber = deviceInfo.hasMap("model");
  std::string manufacturerName = deviceInfo.hasMap("manufacturerName");
  std::string manufacturerUrl = deviceInfo.hasMap("manufacturerUrl");
  std::string manufactureDate = deviceInfo.hasMap("manufacturerDate");
  std::string platformVersion = deviceInfo.hasMap("platformVersion");
  std::string firmwareVersion = deviceInfo.hasMap("firmwareVersion");
  std::string supportUrl = deviceInfo.hasMap("supportUrl");

  std::string systemTime = deviceInfo.hasMap("systemTime");
  if (systemTime == "") systemTime = "default";

  DEBUG_MSG(
      "registerPlatformInfo:\n"
      "\tID:          %s\n"
      "\tmodelNumber: %s\n"
      "\tmanufName:   %s\n"
      "\tmanufUrl:    %s\n"
      "\tmanufDate:   %s\n",
      platformID.c_str(), modelNumber.c_str(), manufacturerName.c_str(),
      manufacturerUrl.c_str(), manufactureDate.c_str());

  DEBUG_MSG(
      "\tplatform:    %s\n"
      "\tOS:          %s\n"
      "\thw:          %s\n"
      "\tfw:          %s\n"
      "\turl:         %s\n"
      "\ttime:        %s\n",
      platformVersion.c_str(), operatingSystemVersion.c_str(),
      hardwareVersion.c_str(), firmwareVersion.c_str(), supportUrl.c_str(),
      systemTime.c_str());

  result = SetPlatformInfo(
      platformInfo, platformID, manufacturerName, manufacturerUrl, modelNumber,
      manufactureDate, platformVersion, operatingSystemVersion, hardwareVersion,
      firmwareVersion, supportUrl, systemTime);
  if (OC_STACK_OK != result) {
    ERROR_MSG("Platform Registration was unsuccessful\n");
    return result;
  }

  result = OCPlatform::registerPlatformInfo(platformInfo);
  if (OC_STACK_OK != result) {
    ERROR_MSG("OCPlatform::registerPlatformInfo was unsuccessful\n");
    return result;
  }

  DeletePlatformInfo(platformInfo);

  return result;
}

OCStackResult IotivityDevice::configureDeviceInfo(
    IotivityDeviceInfo& deviceInfo) {
  OCStackResult result = OC_STACK_ERROR;
  OCDeviceInfo oCDeviceInfo = {0};

  DEBUG_MSG("configureDeviceInfo\n");

  if (deviceInfo.mapSize() == 0) {
    // nothing to do
    return OC_STACK_OK;
  }

  std::string deviceName = deviceInfo.hasMap("name");

  result = SetDeviceInfo(oCDeviceInfo, deviceName);
  result = OCPlatform::registerDeviceInfo(oCDeviceInfo);
  if (OC_STACK_OK != result) {
    ERROR_MSG("OCPlatform::registerDeviceInfo was unsuccessful\n");
    return result;
  }

  return result;
}

void IotivityDevice::handleConfigure(const picojson::value& value) {
  DEBUG_MSG("handleConfigure: v=%s\n", value.serialize().c_str());

  double async_call_id = value.get("asyncCallId").get<double>();

  IotivityDeviceSettings deviceSettings;
  deviceSettings.deserialize(value.get("settings"));

  configure(&deviceSettings);

  OCStackResult result = configurePlatformInfo(deviceSettings.m_deviceInfo);
  if (OC_STACK_OK != result) {
    postError(async_call_id);
    return;
  }

  result = configureDeviceInfo(deviceSettings.m_deviceInfo);
  if (OC_STACK_OK != result) {
    postError(async_call_id);
    return;
  }

  postResult("configureCompleted", async_call_id);
}

static void systemReboot() {
  DEBUG_MSG("systemReboot\n");
  // TODO(aphao) Not sure userspace has enough privilege to reboot.
}

void IotivityDevice::handleFactoryReset(const picojson::value& value) {
  double async_call_id = value.get("asyncCallId").get<double>();
  // TODO(aphao)
  // Return to factory configuration
  systemReboot();
}

void IotivityDevice::handleReboot(const picojson::value& value) {
  double async_call_id = value.get("asyncCallId").get<double>();
  // TODO(aphao)
  // Keep current configuration
  systemReboot();
}

void IotivityDevice::PostMessage(const char* msg) {
  DEBUG_MSG("[Native==>JS] PostMessage: v=%s\n", msg);
  m_instance->PostMessage(msg);
}

void IotivityDevice::postResult(const char* completed_operation,
                                double async_operation_id) {
  DEBUG_MSG("postResult: c=%s, id=%f\n", completed_operation,
            async_operation_id);

  picojson::value::object object;
  object["cmd"] = picojson::value(completed_operation);
  object["asyncCallId"] = picojson::value(async_operation_id);

  picojson::value value(object);
  PostMessage(value.serialize().c_str());
}

void IotivityDevice::postError(double async_operation_id) {
  DEBUG_MSG("postError: id=%d\n", async_operation_id);

  picojson::value::object object;
  object["cmd"] = picojson::value("asyncCallError");
  object["asyncCallId"] = picojson::value(async_operation_id);

  picojson::value value(object);
  PostMessage(value.serialize().c_str());
}

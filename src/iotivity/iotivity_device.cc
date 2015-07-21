/****************************************************************************
**
** Copyright © 1992-2014 Cisco and/or its affiliates. All rights reserved.
** All rights reserved.
**
** $CISCO_BEGIN_LICENSE:APACHE$
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
** http://www.apache.org/licenses/LICENSE-2.0
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
** $CISCO_END_LICENSE$
**
****************************************************************************/
#include "iotivity/iotivity_device.h"
#include "common/extension.h"
#include "iotivity/iotivity_server.h"
#include "iotivity/iotivity_client.h"

IotivityDevice::IotivityDevice(common::Instance* instance) :
  m_instance(instance) {
    m_server = new IotivityServer(this);
    m_client = new IotivityClient(this);
}

IotivityDevice::~IotivityDevice() {
    delete m_server;
    delete m_client;
}

common::Instance* IotivityDevice::getInstance() {     return m_instance; }

IotivityServer* IotivityDevice::getServer() {
    return m_server;
}

IotivityClient* IotivityDevice::getClient() {
    return m_client;
}

static void DuplicateString(char ** targetString, std::string sourceString) {
    *targetString = new char[sourceString.length() + 1];
    strncpy(*targetString, sourceString.c_str(), (sourceString.length() + 1));
}

void DeletePlatformInfo(OCPlatformInfo &platformInfo) {
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

static OCStackResult SetPlatformInfo(OCPlatformInfo &platformInfo,
    std::string platformID, std::string manufacturerName,
    std::string manufacturerUrl, std::string modelNumber,
    std::string dateOfManufacture,
    std::string platformVersion,
    std::string operatingSystemVersion,
    std::string hardwareVersion,
    std::string firmwareVersion,
    std::string supportUrl,
    std::string systemTime) {
    DuplicateString(&platformInfo.platformID, platformID);
    DuplicateString(&platformInfo.manufacturerName, manufacturerName);
    DuplicateString(&platformInfo.manufacturerUrl, manufacturerUrl);
    DuplicateString(&platformInfo.modelNumber, modelNumber);
    DuplicateString(&platformInfo.dateOfManufacture, dateOfManufacture);
    DuplicateString(&platformInfo.platformVersion, platformVersion);
    DuplicateString(&platformInfo.operatingSystemVersion,
        operatingSystemVersion);
    DuplicateString(&platformInfo.hardwareVersion, hardwareVersion);
    DuplicateString(&platformInfo.firmwareVersion, firmwareVersion);
    DuplicateString(&platformInfo.supportUrl, supportUrl);
    DuplicateString(&platformInfo.systemTime, systemTime);
    return OC_STACK_OK;
}

void IotivityDevice::handleConfigure(const picojson::value& value) {
    DEBUG_MSG("handleConfigure: v=%s\n", value.serialize().c_str());

    double async_call_id = value.get("asyncCallId").get<double>();
    std::string deviceRole = value.get("settings").get("role").to_str();
    // TOD(aphao) check for invalid values
    picojson::value param = value.get("settings").get("info");

    std::string platformID = param.get("uuid").to_str();
    std::string manufacturerName = param.get("manufacturerName").to_str();
    std::string manufacturerUrl = param.get("manufacturerUrl").to_str();
    std::string modelNumber = param.get("model").to_str();
    std::string manufactureDate = param.get("manufactureDate").to_str();
    std::string platformVersion = param.get("coreSpecVersion").to_str();
    std::string operatingSystemVersion = param.get("osVersion").to_str();
    std::string hardwareVersion = param.get("hardwareVersion").to_str();
    std::string firmwareVersion = param.get("firmwareVersion").to_str();
    std::string supportUrl = param.get("supportUrl").to_str();
    std::string systemTime = param.get("uuid").to_str();

    OCPlatformInfo platformInfo = {0};
    OCStackResult result = SetPlatformInfo(platformInfo,
                                           platformID,
                                           manufacturerName,
                                           manufacturerUrl,
                                           modelNumber,
                                           manufactureDate,
                                           platformVersion,
                                           operatingSystemVersion,
                                           hardwareVersion,
                                           firmwareVersion,
                                           supportUrl,
                                           systemTime);
    if (OC_STACK_OK != result) {
        std::cerr << "Platform Registration was unsuccessful\n";
        postError(async_call_id);
        return;
    }

    result = OCPlatform::registerPlatformInfo(platformInfo);

    if (OC_STACK_OK != result) {
        std::cerr << "OCPlatform::registerPlatformInfo was unsuccessful\n";
        postError(async_call_id);
        return;
    }

    DeletePlatformInfo(platformInfo);

    // By setting to "0.0.0.0", it binds to all available interfaces
    // Uses randomly available port
    if (deviceRole == "client") {
        PlatformConfig cfg {
            ServiceType::InProc,
            ModeType::Client,
            "0.0.0.0",
            0,
            QualityOfService::LowQos
        };

        OCPlatform::Configure(cfg);
    } else if (deviceRole == "server") {
        PlatformConfig cfg {
            ServiceType::InProc,
            ModeType::Server,
            "0.0.0.0",
            0,
            QualityOfService::LowQos
        };

        OCPlatform::Configure(cfg);
    } else if (deviceRole == "intermediate") {
        PlatformConfig cfg {
            ServiceType::InProc,
            ModeType::Both,
            "0.0.0.0",
            0,
            QualityOfService::LowQos
        };

        OCPlatform::Configure(cfg);
    }
    // TODO(aphao)

    postResult("configureCompleted", async_call_id);
}

void IotivityDevice::handleFactoryReset(const picojson::value& value) {
    double async_call_id = value.get("asyncCallId").get<double>();
    // TODO(aphao)
    // Return to factory configuration + reboot
}

void IotivityDevice::handleReboot(const picojson::value& value) {
    double async_call_id = value.get("asyncCallId").get<double>();
    // TODO(aphao)
    // Keep current configuration + reboot
}


void IotivityDevice::PostMessage(const char *msg) {
    DEBUG_MSG("[Native==>JS] PostMessage: v=%s\n", msg);
    m_instance->PostMessage(msg);
}

void IotivityDevice::postResult(const char* completed_operation,
                                double async_operation_id) {
    DEBUG_MSG("postResult: c=%s, id=%f\n",
              completed_operation, async_operation_id);

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






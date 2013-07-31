// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_build.h"

#include <stdlib.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <fstream>
#include <string>

#include "common/picojson.h"
#include "system_info/system_info_utils.h"

using namespace system_info;

SysInfoBuild::SysInfoBuild(ContextAPI* api)
   : stopping_(false) {
  api_ = api;
}

SysInfoBuild::~SysInfoBuild() {
}

void SysInfoBuild::Get(picojson::value& error,
                       picojson::value& data) {
  // model and manufacturer
  if(!UpdateHardware()) {
    SetPicoJsonObjectValue(error, "message",
        picojson::value("Get Hardware info failed."));
    return;
  }
  SetPicoJsonObjectValue(data, "model", picojson::value(model_));
  SetPicoJsonObjectValue(data, "manufacturer", picojson::value(manufacturer_));

  // build version
  if(!UpdateOSBuild()) {
    SetPicoJsonObjectValue(error, "message",
        picojson::value("Get Build version failed."));
    return;
  }
  SetPicoJsonObjectValue(data, "buildVersion", picojson::value(buildversion_));

  SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

bool SysInfoBuild::UpdateHardware() {
  std::ifstream in;

  in.open("/var/log/dmesg");
  int dmipos;
  std::string info;
  do {
    getline(in, info);
    dmipos = info.find("] DMI: ", 0);
  } while (dmipos == std::string::npos);
  info.erase(0, dmipos + 7);
  in.close();

  int head = 0;
  int tail = -1;
  std::string str;

  // manufacturer
  tail = info.find(' ', 0);
  str.assign(info, head, tail - head);
  if(str.empty()) {
    return false;
  } else {
    manufacturer_.assign(str);
  }

  // model
  head = tail + 1;
  tail = info.find(',', 0);
  str.assign(info, head, tail - head);
  if(str.empty()) {
    return false;
  } else {
    model_.assign(str);
  }

  return true;
}

bool SysInfoBuild::UpdateOSBuild() {
  static struct utsname buf;
  memset(&buf, 0, sizeof (struct utsname));
  uname(&buf);
  char* build_version = strcat(buf.sysname, buf.release);
  if(!build_version) {
    return false;
  } else {
    buildversion_.assign(build_version);
    return true;
  }
}

gboolean SysInfoBuild::TimedOutUpdate(gpointer user_data) {
  SysInfoBuild* instance = static_cast<SysInfoBuild*>(user_data);

  if (instance->stopping_) {
    instance->stopping_ = false;
    return FALSE;
  }

  std::string oldmodel_ = instance->model_;
  std::string oldmanufacturer_ = instance->manufacturer_;
  std::string oldbuildversion_ = instance->buildversion_;
  instance->UpdateHardware();
  instance->UpdateOSBuild();

  if (oldmodel_ != instance->model_ ||
      oldmanufacturer_ != instance->manufacturer_ ||
      oldbuildversion_ != instance->buildversion_) {
    picojson::value output = picojson::value(picojson::object());
    picojson::value data = picojson::value(picojson::object());

    SetPicoJsonObjectValue(data, "manufacturer", picojson::value(instance->manufacturer_));
    SetPicoJsonObjectValue(data, "model", picojson::value(instance->model_));
    SetPicoJsonObjectValue(data, "buildVersion", picojson::value(instance->buildversion_));
    SetPicoJsonObjectValue(output, "cmd",
        picojson::value("SystemInfoPropertyValueChanged"));
    SetPicoJsonObjectValue(output, "prop", picojson::value("BUILD"));
    SetPicoJsonObjectValue(output, "data", data);

    std::string result = output.serialize();
    instance->api_->PostMessage(result.c_str());
  }

  return TRUE;
}

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_build.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <string>

#include "common/picojson.h"

const std::string SysInfoBuild::name_ = "BUILD";

void SysInfoBuild::Get(picojson::value& error,
                       picojson::value& data) {
  // model and manufacturer
  if (!UpdateHardware()) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get Hardware info failed."));
    return;
  }
  system_info::SetPicoJsonObjectValue(data, "model",
      picojson::value(model_));
  system_info::SetPicoJsonObjectValue(data, "manufacturer",
      picojson::value(manufacturer_));

  // build version
  if (!UpdateOSBuild()) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get Build version failed."));
    return;
  }
  system_info::SetPicoJsonObjectValue(data, "buildVersion",
      picojson::value(buildversion_));

  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

bool SysInfoBuild::UpdateHardware() {
  FILE* fp = fopen("/var/log/dmesg", "r");

  int dmipos;
  size_t length = 300;
  char* cinfo = NULL;
  std::string info;

  do {
    getline(&cinfo, &length, fp);
    info = cinfo;
    dmipos = info.find("] DMI: ", 0);
  } while (dmipos == std::string::npos);
  info.erase(0, dmipos + 7);
  free(cinfo);
  fclose(fp);

  int head = 0;
  int tail = -1;
  std::string str;

  // manufacturer
  tail = info.find(' ', 0);
  str.assign(info, head, tail - head);
  if (str.empty()) {
    return false;
  } else {
    manufacturer_ = str;
  }

  // model
  head = tail + 1;
  tail = info.find(',', 0);
  str.assign(info, head, tail - head);
  if (str.empty()) {
    return false;
  } else {
    model_ = str;
  }

  return true;
}

bool SysInfoBuild::UpdateOSBuild() {
  static struct utsname buf;
  memset(&buf, 0, sizeof(struct utsname));
  uname(&buf);

  std::string build_version = std::string(buf.sysname);
  build_version += buf.release;

  if (build_version.empty()) {
    return false;
  } else {
    buildversion_ = build_version;
    return true;
  }
}

gboolean SysInfoBuild::OnUpdateTimeout(gpointer user_data) {
  SysInfoBuild* instance = static_cast<SysInfoBuild*>(user_data);

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

    system_info::SetPicoJsonObjectValue(data, "manufacturer",
        picojson::value(instance->manufacturer_));
    system_info::SetPicoJsonObjectValue(data, "model",
        picojson::value(instance->model_));
    system_info::SetPicoJsonObjectValue(data, "buildVersion",
        picojson::value(instance->buildversion_));
    system_info::SetPicoJsonObjectValue(output, "cmd",
        picojson::value("SystemInfoPropertyValueChanged"));
    system_info::SetPicoJsonObjectValue(output, "prop",
        picojson::value("BUILD"));
    system_info::SetPicoJsonObjectValue(output, "data", data);

    instance->PostMessageToListeners(output);;
  }

  return TRUE;
}

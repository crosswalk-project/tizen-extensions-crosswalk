// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_build.h"

#include <libudev.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <fstream>

#include "common/picojson.h"
#include "system_info/system_info_utils.h"

using namespace system_info;

SysInfoBuild::SysInfoBuild(picojson::value& error) {
  udev_ = udev_new();
  if (!udev_) {
    SetPicoJsonObjectValue(error, "message",
        picojson::value("Can't create udev."));
  }
}

SysInfoBuild::~SysInfoBuild() {
  if(udev_)
    udev_unref(udev_);
}

bool SysInfoBuild::GetBuildInfo(std::string& Manufactor,
                                std::string& Model,
                                std::string& Build) {
  static struct utsname buf;
  memset(&buf, 0, sizeof (struct utsname));
  uname(&buf);
  char *tmpbuild = strcat(buf.sysname, buf.release);
  Build.assign(tmpbuild);

  std::ifstream in;
  in.open("/var/log/dmesg");

  int dmipos;
  std::string info;
  do {
    getline(in, info);
    dmipos = info.find("] DMI: ", 0);
  } while (dmipos == std::string::npos);
  info.erase(0, dmipos + 7);

  int ManufacHead = 0;
  int ManufacTail = -1;
  ManufacTail = info.find(' ', 0);
  Manufactor.assign(info, ManufacHead, ManufacTail - ManufacHead);

  int ModelHead = 0;
  int ModelTail = -1;
  ModelHead = ManufacTail + 1;
  ModelTail = info.find(',', 0);
  Model.assign(info, ModelHead, ModelTail-ModelHead);

  in.close();

  return !(Model.empty() || Manufactor.empty());
}

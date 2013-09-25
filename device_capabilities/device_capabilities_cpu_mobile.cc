// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device_capabilities/device_capabilities_cpu.h"

#include <device.h>
#include <system_info.h>

void DeviceCapabilitiesCpu::Get(picojson::value& obj) {
  if (QueryNumOfProcessors() &&
      QueryArchName() &&
      QueryLoad()) {
    SetJsonValue(obj);
  }
}

void DeviceCapabilitiesCpu::SetJsonValue(picojson::value& obj) {
  picojson::object& o = obj.get<picojson::object>();
  o["numOfProcessors"] = picojson::value(static_cast<double>(numOfProcessors_));
  o["archName"] = picojson::value(archName_);
  o["load"] = picojson::value(load_);
}

bool DeviceCapabilitiesCpu::QueryNumOfProcessors() {
  int numofprocessors = 0;
  int ret = device_cpu_get_count(&numofprocessors);
  if (DEVICE_ERROR_NONE != ret) {
    return false;
  }

  numOfProcessors_ = numofprocessors;
  return true;
}

bool DeviceCapabilitiesCpu::QueryArchName() {
  char *archname = NULL;
  int ret = system_info_get_value_string(SYSTEM_INFO_KEY_CORE_CPU_ARCH,
                                         &archname);
  if (SYSTEM_INFO_ERROR_NONE != ret) {
    free(archname);
    return false;
  }

  archName_ = archname;
  free(archname);
  return true;
}

bool DeviceCapabilitiesCpu::QueryLoad() {
  FILE *fp = fopen("/proc/stat", "r");
  if (!fp)
    return false;

  unsigned long long user; //NOLINT
  unsigned long long nice; //NOLINT
  unsigned long long system; //NOLINT
  unsigned long long idle; //NOLINT
  unsigned long long iowait; //NOLINT
  unsigned long long irq; //NOLINT
  unsigned long long softirq; //NOLINT

  unsigned long long total; //NOLINT
  unsigned long long used; //NOLINT

  if (fscanf(fp, "%*s %llu %llu %llu %llu %llu %llu %llu",
             &user, &nice, &system, &idle, &iowait, &irq, &softirq) != 7) {
      fclose(fp);
      return false;
  }
  fclose(fp);

  // The algorithm here can be found at:
  // http://stackoverflow.com/questions/3017162
  // /how-to-get-total-cpu-usage-in-linux-c
  //
  // The process is:
  // work_over_period = work_jiffies_2 - work_jiffies_1
  // total_over_period = total_jiffies_2 - total_jiffies_1
  // cpu_load = work_over_period / total_over_period
  used = user + nice + system;
  total = used + idle + iowait + irq + softirq;

  load_ = static_cast<double>(used - old_used_) / (total - old_total_);

  old_total_ = total;
  old_used_ = used;

  return true;
}

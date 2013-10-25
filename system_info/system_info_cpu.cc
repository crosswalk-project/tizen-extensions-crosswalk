// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_cpu.h"

#include <stdio.h>
#include <string>

const std::string SysInfoCpu::name_ = "CPU";

void SysInfoCpu::Get(picojson::value& error,
                     picojson::value& data) {
  if (!UpdateLoad()) {
    system_info::SetPicoJsonObjectValue(error, "message",
        picojson::value("Get CPU load failed."));
    return;
  }

  system_info::SetPicoJsonObjectValue(data, "load", picojson::value(load_));
  system_info::SetPicoJsonObjectValue(error, "message", picojson::value(""));
}

gboolean SysInfoCpu::OnUpdateTimeout(gpointer user_data) {
  SysInfoCpu* instance = static_cast<SysInfoCpu*>(user_data);

  double old_load = instance->load_;
  instance->UpdateLoad();
  if (old_load != instance->load_) {
    picojson::value output = picojson::value(picojson::object());
    picojson::value data = picojson::value(picojson::object());

    system_info::SetPicoJsonObjectValue(data, "load",
        picojson::value(instance->load_));
    system_info::SetPicoJsonObjectValue(output, "cmd",
        picojson::value("SystemInfoPropertyValueChanged"));
    system_info::SetPicoJsonObjectValue(output, "prop", picojson::value("CPU"));
    system_info::SetPicoJsonObjectValue(output, "data", data);

    instance->PostMessageToListeners(output);
  }

  return TRUE;
}

void SysInfoCpu::StartListening() {
  if (timeout_cb_id_ == 0) {
    timeout_cb_id_ = g_timeout_add(system_info::default_timeout_interval,
                                   SysInfoCpu::OnUpdateTimeout,
                                   static_cast<gpointer>(this));
  }
}

void SysInfoCpu::StopListening() {
  if (timeout_cb_id_ > 0) {
    g_source_remove(timeout_cb_id_);
    timeout_cb_id_ = 0;
  }
}

bool SysInfoCpu::UpdateLoad() {
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

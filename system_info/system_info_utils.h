// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_UTILS_H_
#define SYSTEM_INFO_SYSTEM_INFO_UTILS_H_

#include <libudev.h>
#include <pthread.h>

#include <list>
#include <string>

#include "common/extension_adapter.h"
#include "common/picojson.h"

typedef std::list<ContextAPI*> SystemInfoEventsList;

static SystemInfoEventsList battery_events_;
static SystemInfoEventsList build_events_;
static SystemInfoEventsList cellular_events_;
static SystemInfoEventsList cpu_events_;
static SystemInfoEventsList device_orientation_events_;
static SystemInfoEventsList display_events_;
static SystemInfoEventsList local_events_;
static SystemInfoEventsList network_events_;
static SystemInfoEventsList peripheral_events_;
static SystemInfoEventsList sim_events_;
static SystemInfoEventsList storage_events_;
static SystemInfoEventsList wifi_network_events_;

struct AutoLock {
  explicit AutoLock(pthread_mutex_t* m) : m_(m) { pthread_mutex_lock(m_); }
  ~AutoLock() { pthread_mutex_unlock(m_); }
 private:
  pthread_mutex_t* m_;
};

namespace system_info {

// The default timeout interval is set to 1s to match the top update interval.
const int default_timeout_interval = 1000;

int ReadOneByte(const char* path);
// Free the returned value after using.
char* ReadOneLine(const char* path);
std::string GetUdevProperty(struct udev_device* dev,
                              const std::string& attr);
void SetPicoJsonObjectValue(picojson::value& obj,
                            const char* prop,
                            const picojson::value& val);
std::string GetPropertyFromFile(const std::string& file_path,
                                const std::string& key);
bool IsExist(const char* path);

}  // namespace system_info

#endif  // SYSTEM_INFO_SYSTEM_INFO_UTILS_H_

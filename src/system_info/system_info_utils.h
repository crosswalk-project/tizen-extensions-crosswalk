// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_UTILS_H_
#define SYSTEM_INFO_SYSTEM_INFO_UTILS_H_

#include <libudev.h>
#include <pthread.h>
#include <unistd.h>

#if defined(TIZEN) && !defined(TIZEN_MOBILE)
#include <gio/gio.h>
#include <glib.h>
#include <glib-object.h>
#endif
#include <string>

#include "common/picojson.h"

struct AutoLock {
  explicit AutoLock(pthread_mutex_t* m) : m_(m) { pthread_mutex_lock(m_); }
  ~AutoLock() { pthread_mutex_unlock(m_); }
 private:
  pthread_mutex_t* m_;
};

namespace system_info {

// The default timeout interval is set to 1s to match the top update interval.
const int default_timeout_interval = 1000;
#ifdef TIZEN
char* GetDuidProperty();
#ifndef TIZEN_MOBILE
const char kOfonoService[] = "org.ofono";
const char kOfonoManagerPath[] = "/";
const char kOfonoManagerIface[] = "org.ofono.Manager";
const char kOfonoModemIface[] = "org.ofono.Modem";
const char kOfonoNetworkRegistrationIface[] = "org.ofono.NetworkRegistration";
const char kOfonoConnectionManagerIface[] = "org.ofono.ConnectionManager";
const char kOfonoConnectionContextIface[] = "org.ofono.ConnectionContext";
const char kOfonoSimManagerIface[] = "org.ofono.SimManager";

GDBusConnection* GetDbusConnection();
std::string OfonoGetModemPath(GDBusConnection* bus_conn);
#endif  // TIZEN_MOBILE
#endif  // TIZEN
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
inline bool PathExists(const char* path) {
  return 0 == access(path, F_OK);
}
inline bool ParseBoolean(std::string str) {
  return str == "true" ? true : false;
}

}  // namespace system_info

#endif  // SYSTEM_INFO_SYSTEM_INFO_UTILS_H_

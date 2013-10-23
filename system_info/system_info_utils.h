// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_UTILS_H_
#define SYSTEM_INFO_SYSTEM_INFO_UTILS_H_

#include <libudev.h>
#include <pthread.h>
#include <unistd.h>

#include <list>
#include <string>

#include "common/extension_adapter.h"
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

class SysInfoObject {
 public:
  SysInfoObject() {
    pthread_mutex_init(&listeners_mutex_, NULL);
  }

  ~SysInfoObject() {
    AutoLock* lock = new AutoLock(&listeners_mutex_);
    for (std::list<ContextAPI*>::iterator it = listeners_.begin();
         it != listeners_.end(); it++) {
      RemoveListener(*it);
    }
    delete lock;
    pthread_mutex_destroy(&listeners_mutex_);
  }

  // Get support
  virtual void Get(picojson::value& error, picojson::value& data) = 0;

  // Listener support
  virtual void AddListener(ContextAPI* api) = 0;
  virtual void RemoveListener(ContextAPI* api) {}

  void PostMessageToListeners(const picojson::value& output) {
    AutoLock lock(&listeners_mutex_);
    std::string result = output.serialize();
    for (std::list<ContextAPI*>::iterator it = listeners_.begin();
         it != listeners_.end(); it++) {
      (*it)->PostMessage(result.c_str());
    }
  }

 protected:
  pthread_mutex_t listeners_mutex_;
  std::list<ContextAPI*> listeners_;
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_UTILS_H_

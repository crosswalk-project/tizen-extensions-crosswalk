// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_INSTANCE_H_
#define SYSTEM_INFO_SYSTEM_INFO_INSTANCE_H_

#include <list>
#include <map>
#include <string>
#include <utility>

#include "common/extension.h"
#include "common/picojson.h"
#include "system_info/system_info_utils.h"

namespace picojson {
class value;
}  // namespace picojson

class SystemInfoInstance : public common::Instance {
 public:
  ~SystemInfoInstance();
  static void InstancesMapInitialize();

 private:
  // common::Instance implementation.
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* msg);

  void HandleGetPropertyValue(const picojson::value& input,
                              picojson::value& output);
  void HandleStartListening(const picojson::value& input);
  void HandleStopListening(const picojson::value& input);
  void HandleGetCapabilities();
  inline void SetStringPropertyValue(picojson::object& o,
                                     const char* prop,
                                     const char* val) {
    if (val)
      o[prop] = picojson::value(val);
  }

  template <class T>
  static void RegisterClass();
};

class SysInfoObject {
 public:
  SysInfoObject() {
    pthread_mutex_init(&listeners_mutex_, NULL);
  }

  ~SysInfoObject() {
    AutoLock* lock = new AutoLock(&listeners_mutex_);
    for (std::list<SystemInfoInstance*>::iterator it = listeners_.begin();
         it != listeners_.end(); it++) {
      RemoveListener(*it);
    }
    delete lock;
    pthread_mutex_destroy(&listeners_mutex_);
  }

  // Get support
  virtual void Get(picojson::value& error, picojson::value& data) = 0;

  // Listener support
  void AddListener(SystemInfoInstance* instance) {
    AutoLock lock(&listeners_mutex_);
    listeners_.push_back(instance);

    if (listeners_.size() > 1)
      return;
    StartListening();
  }
  void RemoveListener(SystemInfoInstance* instance) {
    AutoLock lock(&listeners_mutex_);
    listeners_.remove(instance);

    if (!listeners_.empty())
      return;
    StopListening();
  }
  virtual void StartListening() {}
  virtual void StopListening() {}
  void PostMessageToListeners(const picojson::value& output) {
    AutoLock lock(&listeners_mutex_);
    std::string result = output.serialize();
    for (std::list<SystemInfoInstance*>::iterator it = listeners_.begin();
         it != listeners_.end(); it++) {
      (*it)->PostMessage(result.c_str());
    }
  }

 protected:
  pthread_mutex_t listeners_mutex_;
  std::list<SystemInfoInstance*> listeners_;
};

typedef std::map<std::string, SysInfoObject&> SysInfoClassMap;
typedef SysInfoClassMap::iterator classes_iterator;
typedef std::pair<std::string, SysInfoObject&> SysInfoClassPair;
static SysInfoClassMap classes_;

#endif  // SYSTEM_INFO_SYSTEM_INFO_INSTANCE_H_

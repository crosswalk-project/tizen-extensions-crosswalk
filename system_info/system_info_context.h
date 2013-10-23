// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_CONTEXT_H_
#define SYSTEM_INFO_SYSTEM_INFO_CONTEXT_H_

#include <map>
#include <string>
#include <utility>

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "system_info/system_info_utils.h"

typedef std::map<std::string, SysInfoObject&> SysInfoClassMap;
typedef SysInfoClassMap::iterator classes_iterator;
typedef std::pair<std::string, SysInfoObject&> SysInfoClassPair;

namespace picojson {
class value;
}  // namespace picojson

class SystemInfoContext {
 public:
  explicit SystemInfoContext(ContextAPI* api);
  ~SystemInfoContext();

  // ExtensionAdapter implementation.
  static const char name[];
  static const char* GetJavaScript();
  static SysInfoClassMap classes_;
  void HandleMessage(const char* message);
  void HandleSyncMessage(const char* message);
  static void InstancesMapInitialize();

 private:
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
  ContextAPI* api_;
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_CONTEXT_H_

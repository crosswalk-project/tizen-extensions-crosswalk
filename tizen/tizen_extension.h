// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TIZEN_TIZEN_EXTENSION_H_
#define TIZEN_TIZEN_EXTENSION_H_

#include "common/extension.h"
#include "common/picojson.h"

class TizenExtension : public common::Extension {
 public:
  TizenExtension();
  virtual ~TizenExtension();

  virtual common::Instance* CreateInstance();

  std::string GetRuntimeVariable(const std::string& key);
};

class TizenExtensionInstance : public common::Instance {
 public:
  TizenExtensionInstance(TizenExtension* extension) : extension_(extension) {}

  virtual void HandleSyncMessage(const char* msg);

 private:
  void HandleGetRuntimeVariable(const picojson::value& input);

  TizenExtension* extension_;
};

#endif  // TIZEN_TIZEN_EXTENSION_H_

// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "audiosystem/audiosystem_extension.h"

#include "audiosystem/audiosystem_instance.h"
#include "audiosystem/audiosystem_logs.h"

#ifndef NDEBUG
std::ofstream _as_f_log;
#endif  // NDEBUG

common::Extension* CreateExtension() {
  DBG("Extension created");
  return new AudioSystemExtension;
}

// This will be generated from audiosystem_api.js.
extern const char kSource_audiosystem_api[];

AudioSystemExtension::AudioSystemExtension() {
  LOG_INIT();
  DBG("Initializing extension");
  SetExtensionName("tizen.audiosystem");
  SetJavaScriptAPI(kSource_audiosystem_api);
}

AudioSystemExtension::~AudioSystemExtension() {
  DBG("Un-initializing extension");
  LOG_CLOSE();
}

common::Instance* AudioSystemExtension::CreateInstance() {
  return new AudioSystemInstance();
}

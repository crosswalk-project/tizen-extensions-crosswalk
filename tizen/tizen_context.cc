// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen/tizen_context.h"

CXWalkExtension* xwalk_extension_init(int32_t api_version) {
  return ExtensionAdapter<TizenContext>::Initialize();
}

TizenContext::TizenContext(ContextAPI* api) : api_(api) {}

TizenContext::~TizenContext() {
  delete api_;
}

const char TizenContext::name[] = "tizen";

// This will be generated from tizen_api.js.
extern const char kSource_tizen_api[];

const char* TizenContext::GetJavaScript() {
  return kSource_tizen_api;
}

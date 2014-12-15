// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utils/utils_extension.h"

// This will be generated from tizen_api.js.
extern const char kSource_utils_api[];

common::Extension* CreateExtension() {
  return new UtilsExtension;
}

UtilsExtension::UtilsExtension() {
  SetExtensionName("xwalk.utils");
  SetJavaScriptAPI(kSource_utils_api);
}

UtilsExtension::~UtilsExtension() {}

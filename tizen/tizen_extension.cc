// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen/tizen_extension.h"

// This will be generated from tizen_api.js.
extern const char kSource_tizen_api[];

common::Extension* CreateExtension() {
  return new TizenExtension;
}

TizenExtension::TizenExtension() {
  SetExtensionName("tizen");
  SetJavaScriptAPI(kSource_tizen_api);
}

TizenExtension::~TizenExtension() {}

// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>

#include "content/content_extension.h"
#include "content/content_instance.h"

common::Extension* CreateExtension() {
  return new ContentExtension;
}

// JS source code for the API
extern const char kSource_content_api[];

ContentExtension::ContentExtension() {
  SetExtensionName("tizen.content");
  SetJavaScriptAPI(kSource_content_api);
}

ContentExtension::~ContentExtension() {}

common::Instance* ContentExtension::CreateInstance() {
  return new ContentInstance;
}

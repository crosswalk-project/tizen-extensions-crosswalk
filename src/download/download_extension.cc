// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "download/download_extension.h"

#include "download/download_instance.h"

common::Extension* CreateExtension() {
  return new DownloadExtension;
}

// This will be generated from download_api.js.
extern const char kSource_download_api[];

DownloadExtension::DownloadExtension() {
  const char* entry_points[] = { "tizen.DownloadRequest", NULL };
  SetExtraJSEntryPoints(entry_points);
  SetExtensionName("tizen.download");
  SetJavaScriptAPI(kSource_download_api);
}

DownloadExtension::~DownloadExtension() {}

common::Instance* DownloadExtension::CreateInstance() {
  return new DownloadInstance;
}

// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "filesystem/filesystem_extension.h"
#include "filesystem/filesystem_instance.h"

common::Extension* CreateExtension() {
  return new FilesystemExtension();
}

extern const char kSource_filesystem_api[];

FilesystemExtension::FilesystemExtension() {
  SetExtensionName("tizen.filesystem");
  SetJavaScriptAPI(kSource_filesystem_api);
}

FilesystemExtension::~FilesystemExtension() {}

common::Instance* FilesystemExtension::CreateInstance() {
  return new FilesystemInstance();
}

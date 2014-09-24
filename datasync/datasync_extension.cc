// Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "datasync/datasync_extension.h"
#include "datasync/datasync_instance.h"

// This will be generated from datasync_api.js.
extern const char kSource_datasync_api[];

namespace datasync {

DatasyncExtension::DatasyncExtension() {
  SetExtensionName("tizen.datasync");
  SetJavaScriptAPI(kSource_datasync_api);

  const char* entry_points[] = {
      "tizen.SyncInfo", "tizen.SyncProfileInfo", "tizen.SyncServiceInfo", NULL};
  SetExtraJSEntryPoints(entry_points);
}

DatasyncExtension::~DatasyncExtension() {}

common::Instance* DatasyncExtension::CreateInstance() {
  return new DatasyncInstance;
}

}  // namespace datasync

// entry point
common::Extension* CreateExtension() {
  return new datasync::DatasyncExtension;
}

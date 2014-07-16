// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mediaserver/mediaserver_extension.h"

#include "mediaserver/mediaserver_instance.h"

common::Extension* CreateExtension() {
  return new MediaServerExtension;
}

extern const char kSource_mediaserver_api[];

MediaServerExtension::MediaServerExtension() {
  SetExtensionName("navigator.mediaServer");
  SetJavaScriptAPI(kSource_mediaserver_api);
}

MediaServerExtension::~MediaServerExtension() {}

common::Instance* MediaServerExtension::CreateInstance() {
  return new MediaServerInstance;
}

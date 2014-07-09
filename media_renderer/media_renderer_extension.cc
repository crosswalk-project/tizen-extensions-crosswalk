// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media_renderer/media_renderer_extension.h"

#include "media_renderer/media_renderer_instance.h"

common::Extension* CreateExtension() {
  return new MediaRendererExtension;
}

extern const char kSource_media_renderer_api[];

MediaRendererExtension::MediaRendererExtension() {
  SetExtensionName("navigator.mediaRenderer");
  SetJavaScriptAPI(kSource_media_renderer_api);
}

MediaRendererExtension::~MediaRendererExtension() {}

common::Instance* MediaRendererExtension::CreateInstance() {
  return new MediaRendererInstance;
}

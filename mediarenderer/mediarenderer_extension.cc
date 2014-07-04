// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mediarenderer/mediarenderer_extension.h"

#include <glib-object.h>
#include "mediarenderer/mediarenderer_instance.h"

common::Extension* CreateExtension() {
  return new MediaRendererExtension;
}

extern const char kSource_mediarenderer_api[];

MediaRendererExtension::MediaRendererExtension() {
  SetExtensionName("navigator.mediaRenderer");
  SetJavaScriptAPI(kSource_mediarenderer_api);
}

MediaRendererExtension::~MediaRendererExtension() {}

common::Instance* MediaRendererExtension::CreateInstance() {
  return new MediaRendererInstance;
}

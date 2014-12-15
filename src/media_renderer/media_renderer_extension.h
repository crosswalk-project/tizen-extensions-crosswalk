// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_RENDERER_MEDIA_RENDERER_EXTENSION_H_
#define MEDIA_RENDERER_MEDIA_RENDERER_EXTENSION_H_

#include "common/extension.h"

class MediaRendererExtension : public common::Extension {
 public:
  MediaRendererExtension();
  virtual ~MediaRendererExtension();

 private:
  // common::Extension implementation.
  virtual common::Instance* CreateInstance();
};

#endif  // MEDIA_RENDERER_MEDIA_RENDERER_EXTENSION_H_

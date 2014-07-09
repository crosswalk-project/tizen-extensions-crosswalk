// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_RENDERER_MEDIA_RENDERER_INSTANCE_H_
#define MEDIA_RENDERER_MEDIA_RENDERER_INSTANCE_H_

#include <glib.h>
#include <thread>  // NOLINT

#include "common/extension.h"

class MediaRendererManager;

namespace picojson {

class value;

}  // namespace picojson

class MediaRendererInstance : public common::Instance {
 public:
  MediaRendererInstance();
  virtual ~MediaRendererInstance();

 private:
  void InitWorkerThread();
  static gboolean CreateMediaRendererManager(void* data);
  // common::Instance implementation.
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* msg);

  std::thread worker_thread_;
  GMainLoop* worker_loop_ = NULL;
  MediaRendererManager* media_renderer_manager_ = NULL;
};

#endif  // MEDIA_RENDERER_MEDIA_RENDERER_INSTANCE_H_

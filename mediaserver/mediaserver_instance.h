// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIASERVER_MEDIASERVER_INSTANCE_H_
#define MEDIASERVER_MEDIASERVER_INSTANCE_H_

#include <glib.h>
#include <thread>  // NOLINT

#include "common/extension.h"

class MediaServerManager;

namespace picojson {
class value;
}

class MediaServerInstance : public common::Instance {
 public:
  MediaServerInstance();
  virtual ~MediaServerInstance();

 private:
  void InitWorkerThread();
  static gboolean CreateMediaServerManager(void* data);
  // common::Instance implementation.
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* msg);

 private:
  std::thread worker_thread_;
  GMainLoop* worker_loop_ = NULL;
  MediaServerManager* media_server_manager_ = NULL;
};

#endif  // MEDIASERVER_MEDIASERVER_INSTANCE_H_

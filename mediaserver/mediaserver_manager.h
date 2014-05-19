// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIASERVER_MEDIASERVER_MANAGER_H_
#define MEDIASERVER_MEDIASERVER_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include "mediaserver/dleyna_manager_gen.h"
#include "mediaserver/mediaserver.h"
#include "mediaserver/callbacks.h"

namespace common {
class Instance;
}

typedef std::shared_ptr<MediaServer> MediaServerPtr;

class MediaServerManager {
 public:
  explicit MediaServerManager(common::Instance* instance);
  virtual ~MediaServerManager();

  void scanNetwork();
  void getServers(const picojson::value& value);
  void handleBrowse(const picojson::value& value);
  void handleFind(const picojson::value& value);
  void handleCreateFolder(const picojson::value& value);
  void handleUpload(const picojson::value& value);
  void handleCancel(const picojson::value& value);
  void handleUploadToContainer(const picojson::value& value);
  void handleCreateFolderInContainer(const picojson::value& value);

 private:
  void postServerFound(const std::string& path);
  MediaServerPtr getMediaServerById(const picojson::value& id);
  MediaServerPtr getMediaServerById(const std::string& id);
  bool isCancelled() { return g_cancellable_is_cancelled(cancellable_); }

  CALLBACK_METHOD(OnScanNetwork, GObject*, GAsyncResult*, MediaServerManager);
  CALLBACK_METHOD_WITH_ID(OnGetServers, GObject*, GAsyncResult*,
      MediaServerManager);
  CALLBACK_METHOD(OnLostServer, dleynaManager*,
                  const gchar*, MediaServerManager);
  CALLBACK_METHOD(OnFoundServer, dleynaManager*,
                  const gchar*, MediaServerManager);

 private:
  common::Instance* instance_;
  dleynaManager* manager_proxy_;
  std::map<std::string, MediaServerPtr> media_servers_;
  GCancellable* cancellable_;
};

#endif  // MEDIASERVER_MEDIASERVER_MANAGER_H_

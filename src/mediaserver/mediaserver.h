// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIASERVER_MEDIASERVER_H_
#define MEDIASERVER_MEDIASERVER_H_

namespace common {
class Instance;
}

#include <string>
#include "common/picojson.h"
#include "mediaserver/callbacks.h"
#include "mediaserver/dleyna_media_device_gen.h"
#include "mediaserver/upnp_media_container_gen.h"

class MediaServer {
 public:
  MediaServer(common::Instance* instance, const std::string& object_path);
  virtual ~MediaServer();

  void browse(const picojson::value& value);
  void find(const picojson::value& value);
  void createFolder(const picojson::value& value);
  void upload(const picojson::value& value);
  void cancel(const picojson::value& value);
  bool isCancelled() const;

  // MediaContainer methods
  void uploadToContainer(const picojson::value& value);
  void createFolderInContainer(const picojson::value& value);

  picojson::value toJSON();

 private:
  picojson::value mediaObjectToJSON(GVariant* variant);

  void postResult(GVariant* objects,
                  const char* completed_operation,
                  double async_operation_id);
  void postResult(const char* completed_operation,
                  double async_operation_id);
  void postError(double async_operation_id);

  CALLBACK_METHOD_WITH_ID(OnBrowse, GObject*, GAsyncResult*, MediaServer);
  CALLBACK_METHOD_WITH_ID(OnFind, GObject*, GAsyncResult*, MediaServer);
  CALLBACK_METHOD_WITH_ID(OnCreateFolder, GObject*, GAsyncResult*, MediaServer);
  CALLBACK_METHOD_WITH_ID(OnUpload, GObject*, GAsyncResult*, MediaServer);
  CALLBACK_METHOD_WITH_ID(OnUploadToContainer, GObject*,
                          GAsyncResult*, MediaServer);
  CALLBACK_METHOD_WITH_ID(OnCreateFolderInContainer, GObject*,
                          GAsyncResult*, MediaServer);

 private:
  common::Instance* instance_;
  picojson::value::object object_;
  dleynaMediaDevice* mediadevice_proxy_;
  std::string object_path_;
  GCancellable* cancellable_;
};

#endif  // MEDIASERVER_MEDIASERVER_H_

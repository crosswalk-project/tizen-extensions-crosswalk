// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_RENDERER_MEDIA_RENDERER_MANAGER_H_
#define MEDIA_RENDERER_MEDIA_RENDERER_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include "media_renderer/dleyna_manager_gen.h"
#include "media_renderer/media_renderer.h"
#include "media_renderer/callbacks.h"

namespace common {

class Instance;

}  // namespace common

typedef std::shared_ptr<MediaRenderer> MediaRendererPtr;

class MediaRendererManager {
 public:
  explicit MediaRendererManager(common::Instance* instance);
  virtual ~MediaRendererManager();

  void ScanNetwork();
  void GetRenderers(const picojson::value& value);
  void HandleOpenURI(const picojson::value& value);
  void HandlePrefetchURI(const picojson::value& value);
  void HandleCancel(const picojson::value& value);
  void HandlePlay(const picojson::value& value);
  void HandlePause(const picojson::value& value);
  void HandleStop(const picojson::value& value);
  void HandleNext(const picojson::value& value);
  void HandlePrevious(const picojson::value& value);
  void HandleMute(const picojson::value& value);
  void HandleSetSpeed(const picojson::value& value);
  void HandleSetVolume(const picojson::value& value);
  void HandleGotoTrack(const picojson::value& value);

 private:
  void PostRendererFound(const std::string& path);
  MediaRendererPtr GetMediaRendererById(const picojson::value& id);
  MediaRendererPtr GetMediaRendererById(const std::string& id);
  bool IsCancelled() { return g_cancellable_is_cancelled(cancellable_); }

  CALLBACK_METHOD(OnScanNetwork, GObject*, GAsyncResult*, MediaRendererManager);
  CALLBACK_METHOD_WITH_ID(OnGetRenderers, GObject*, GAsyncResult*,
      MediaRendererManager);
  CALLBACK_METHOD(OnLostRenderer, dleynaManager*,
                  const gchar*, MediaRendererManager);
  CALLBACK_METHOD(OnFoundRenderer, dleynaManager*,
                  const gchar*, MediaRendererManager);

 private:
  common::Instance* instance_;
  dleynaManager* manager_proxy_;
  std::map<std::string, MediaRendererPtr> media_renderers_;
  GCancellable* cancellable_;
};

#endif  // MEDIA_RENDERER_MEDIA_RENDERER_MANAGER_H_

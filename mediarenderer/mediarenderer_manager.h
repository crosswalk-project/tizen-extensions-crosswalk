// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIARENDERER_MEDIARENDERER_MANAGER_H_
#define MEDIARENDERER_MEDIARENDERER_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include "mediarenderer/dleyna_manager_gen.h"
#include "mediarenderer/mediarenderer.h"
#include "mediarenderer/callbacks.h"

namespace common {
class Instance;
}

typedef std::shared_ptr<MediaRenderer> MediaRendererPtr;

class MediaRendererManager {
 public:
  explicit MediaRendererManager(common::Instance* instance);
  virtual ~MediaRendererManager();

  void scanNetwork();
  void getRenderers(const picojson::value& value);
  void handleOpenURI(const picojson::value& value);
  void handlePrefetchURI(const picojson::value& value);
  void handleCancel(const picojson::value& value);  
  void handlePlay(const picojson::value& value);
  void handlePause(const picojson::value& value);
  void handleStop(const picojson::value& value);
  void handleNext(const picojson::value& value);
  void handlePrevious(const picojson::value& value);
  void handleMute(const picojson::value& value);
  void handleSetSpeed(const picojson::value& value);
  void handleSetVolume(const picojson::value& value);
  void handleGotoTrack(const picojson::value& value);

 private:
  void postRendererFound(const std::string& path);
  MediaRendererPtr getMediaRendererById(const picojson::value& id);
  MediaRendererPtr getMediaRendererById(const std::string& id);
  bool isCancelled() { return g_cancellable_is_cancelled(cancellable_); }

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

#endif  // MEDIARENDERER_MEDIARENDERER_MANAGER_H_

// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIARENDERER_MEDIARENDERER_H_
#define MEDIARENDERER_MEDIARENDERER_H_

namespace common {
class Instance;
}

#include <string>
#include "common/picojson.h"
#include "mediarenderer/callbacks.h"
#include "mediarenderer/dleyna_renderer_device_gen.h"
#include "mediarenderer/dleyna_pushhost_gen.h"
#include "mediarenderer/mpris_mediaplayer2_gen.h"
#include "mediarenderer/mpris_mediaplayer2_player_gen.h"

class MediaRenderer {
 public:
  MediaRenderer(common::Instance* instance, const std::string& object_path);
  virtual ~MediaRenderer();

  void openURI(const picojson::value& value);
  void prefetchURI(const picojson::value& value);
  void cancel(const picojson::value& value);
  bool isCancelled() const;
  
  // MediaController methods
  void play(const picojson::value& value);
  void pause(const picojson::value& value);
  void stop(const picojson::value& value);
  void next(const picojson::value& value);
  void previous(const picojson::value& value);
  void mute(const picojson::value& value);
  void setSpeed(const picojson::value& value);
  void setVolume(const picojson::value& value);
  void gotoTrack(const picojson::value& value);

  picojson::value toJSON();

 private:

  void postResult(const char* completed_operation,
                  double async_operation_id);
  void postError(double async_operation_id);

  CALLBACK_METHOD_WITH_ID(OnOpenURI, GObject*, GAsyncResult*, MediaRenderer);
  CALLBACK_METHOD_WITH_ID(OnPrefetchURI, GObject*, GAsyncResult*, MediaRenderer);
  CALLBACK_METHOD_WITH_ID(OnCancel, GObject*, GAsyncResult*, MediaRenderer);
  CALLBACK_METHOD_WITH_ID(OnHostFile, GObject*, GAsyncResult*, MediaRenderer);

  CALLBACK_METHOD_WITH_ID(OnPlay, GObject*, GAsyncResult*, MediaRenderer);
  CALLBACK_METHOD_WITH_ID(OnPause, GObject*, GAsyncResult*, MediaRenderer);
  CALLBACK_METHOD_WITH_ID(OnStop, GObject*, GAsyncResult*, MediaRenderer);
  CALLBACK_METHOD_WITH_ID(OnNext, GObject*, GAsyncResult*, MediaRenderer);
  CALLBACK_METHOD_WITH_ID(OnPrevious, GObject*, GAsyncResult*, MediaRenderer);
  CALLBACK_METHOD_WITH_ID(OnGoToTrack, GObject*, GAsyncResult*, MediaRenderer);

 private:
  common::Instance* instance_;
  picojson::value::object object_;
  dleynaRendererDevice* rendererdevice_proxy_;
  dleynaPushHost* pushhost_proxy_;  
  mprisMediaPlayer2* mediaplayer2_proxy_;
  mprismediaplayer2Player* mprisplayer_proxy_;
  std::string object_path_;
  GCancellable* cancellable_;
};

#endif  // MEDIARENDERER_MEDIARENDERER_H_

// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_RENDERER_MEDIA_RENDERER_H_
#define MEDIA_RENDERER_MEDIA_RENDERER_H_

namespace common {

class Instance;

}  // namespace common

#include <string>
#include "common/picojson.h"
#include "media_renderer/callbacks.h"
#include "media_renderer/dleyna_renderer_device_gen.h"
#include "media_renderer/dleyna_pushhost_gen.h"
#include "media_renderer/mpris_mediaplayer2_gen.h"
#include "media_renderer/mpris_mediaplayer2_player_gen.h"

class MediaRenderer {
 public:
  MediaRenderer(common::Instance* instance, const std::string& object_path);
  virtual ~MediaRenderer();

  void OpenURI(const picojson::value& value);
  void PrefetchURI(const picojson::value& value);
  void Cancel(const picojson::value& value);
  bool IsCancelled() const;

  // MediaController methods
  void Play(const picojson::value& value);
  void Pause(const picojson::value& value);
  void Stop(const picojson::value& value);
  void Next(const picojson::value& value);
  void Previous(const picojson::value& value);
  void Mute(const picojson::value& value);
  void SetSpeed(const picojson::value& value);
  void SetVolume(const picojson::value& value);
  void GotoTrack(const picojson::value& value);

  picojson::value ToJSON();

 private:
  void PostResult(const std::string& completed_operation,
                  double async_operation_id);
  void PostError(double async_operation_id);
  static void PropertyChanged(GDBusProxy* proxy, GVariant*, GStrv, gpointer);
  CALLBACK_METHOD_WITH_ID(OnOpenURI, GObject*, GAsyncResult*, MediaRenderer);
  CALLBACK_METHOD_WITH_ID(OnPrefetchURI, GObject*,
      GAsyncResult*, MediaRenderer);
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

#endif  // MEDIA_RENDERER_MEDIA_RENDERER_H_

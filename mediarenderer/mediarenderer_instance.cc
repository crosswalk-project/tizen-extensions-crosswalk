// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mediarenderer/mediarenderer_instance.h"

#include <string>

#include "common/picojson.h"
#include "mediarenderer/mediarenderer_manager.h"

MediaRendererInstance::MediaRendererInstance()
    : worker_thread_(&MediaRendererInstance::InitWorkerThread, this) {
  worker_thread_.detach();
}

MediaRendererInstance::~MediaRendererInstance() {
  g_main_loop_quit(worker_loop_);
  delete media_renderer_manager_;
}

gboolean MediaRendererInstance::CreateMediaRendererManager(void* data) {
  if (!data) {
    std::cerr << "Null pointer is passed to callback" << std::endl;
    return FALSE;
  }

  MediaRendererInstance* instance = static_cast<MediaRendererInstance*>(data);
  instance->media_renderer_manager_ = new MediaRendererManager(instance);
  return FALSE;
}

void MediaRendererInstance::InitWorkerThread() {
  GMainContext* context = g_main_context_default();
  worker_loop_ = g_main_loop_new(context, FALSE);
  g_main_context_push_thread_default(context);
  GSource* source = g_idle_source_new();
  g_source_set_callback(source,
                        &MediaRendererInstance::CreateMediaRendererManager,
                        this,
                        NULL);
  g_source_attach(source, context);
  g_main_loop_run(worker_loop_);
  g_source_destroy(source);
  g_source_unref(source);
  g_main_loop_unref(worker_loop_);
}

void MediaRendererInstance::HandleMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "scanNetwork")
    media_renderer_manager_->scanNetwork();
  else if (cmd == "getRenderers")
    media_renderer_manager_->getRenderers(v);
  else if (cmd == "openURI")
    media_renderer_manager_->handleOpenURI(v);
  else if (cmd == "prefetchURI")
    media_renderer_manager_->handlePrefetchURI(v);
  else if (cmd == "cancel")
    media_renderer_manager_->handleCancel(v);
  else if (cmd == "play")
    media_renderer_manager_->handlePlay(v);
  else if (cmd == "pause")
    media_renderer_manager_->handlePause(v);
  else if (cmd == "stop")
    media_renderer_manager_->handleStop(v);
  else if (cmd == "next")
    media_renderer_manager_->handleNext(v);
  else if (cmd == "previous")
    media_renderer_manager_->handlePrevious(v);
  else if (cmd == "mute")
    media_renderer_manager_->handleMute(v);
  else if (cmd == "setSpeed")
    media_renderer_manager_->handleSetSpeed(v);
  else if (cmd == "setVolume")
    media_renderer_manager_->handleSetVolume(v);
  else if (cmd == "gotoTrack")
    media_renderer_manager_->handleGotoTrack(v);
  else
    std::cerr << "Received unknown message: " << cmd << "\n";
}

void MediaRendererInstance::HandleSyncMessage(const char* message) {}

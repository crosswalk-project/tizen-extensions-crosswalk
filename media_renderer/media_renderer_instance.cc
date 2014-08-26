// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media_renderer/media_renderer_instance.h"

#include <string>

#include "common/picojson.h"
#include "media_renderer/media_renderer_manager.h"

MediaRendererInstance::MediaRendererInstance() {
  media_renderer_manager_ = new MediaRendererManager(this);
}

MediaRendererInstance::~MediaRendererInstance() {
  delete media_renderer_manager_;
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
    media_renderer_manager_->ScanNetwork();
  else if (cmd == "getRenderers")
    media_renderer_manager_->GetRenderers(v);
  else if (cmd == "openURI")
    media_renderer_manager_->HandleOpenURI(v);
  else if (cmd == "prefetchURI")
    media_renderer_manager_->HandlePrefetchURI(v);
  else if (cmd == "cancel")
    media_renderer_manager_->HandleCancel(v);
  else if (cmd == "play")
    media_renderer_manager_->HandlePlay(v);
  else if (cmd == "pause")
    media_renderer_manager_->HandlePause(v);
  else if (cmd == "stop")
    media_renderer_manager_->HandleStop(v);
  else if (cmd == "next")
    media_renderer_manager_->HandleNext(v);
  else if (cmd == "previous")
    media_renderer_manager_->HandlePrevious(v);
  else if (cmd == "mute")
    media_renderer_manager_->HandleMute(v);
  else if (cmd == "setSpeed")
    media_renderer_manager_->HandleSetSpeed(v);
  else if (cmd == "setVolume")
    media_renderer_manager_->HandleSetVolume(v);
  else if (cmd == "gotoTrack")
    media_renderer_manager_->HandleGotoTrack(v);
  else
    std::cerr << "Received unknown message: " << cmd << "\n";
}

void MediaRendererInstance::HandleSyncMessage(const char* message) {}

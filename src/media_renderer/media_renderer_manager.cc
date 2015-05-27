// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media_renderer/media_renderer_manager.h"

#include <utility>
#include <string>
#include "common/extension.h"
#include "media_renderer/media_renderer.h"

typedef std::pair<std::string, std::shared_ptr<MediaRenderer>>
    MediaRendererPair;

MediaRendererManager::MediaRendererManager(common::Instance* instance)
    : instance_(instance) {
  GError* gerror = NULL;
  manager_proxy_ = dleyna_manager_proxy_new_for_bus_sync(
      G_BUS_TYPE_SESSION,
      G_DBUS_PROXY_FLAGS_NONE,
      "com.intel.dleyna-renderer",
      "/com/intel/dLeynaRenderer",
      NULL,
      &gerror);

  if (gerror) {
    g_error_free(gerror);
    return;
  }

  g_signal_connect(
      manager_proxy_,
      "found-renderer",
      G_CALLBACK(OnFoundRendererCallBack),
      this);

  g_signal_connect(
      manager_proxy_,
      "lost-renderer",
      G_CALLBACK(OnLostRendererCallBack),
      this);
}

MediaRendererManager::~MediaRendererManager() {
  g_object_unref(manager_proxy_);
}

void MediaRendererManager::ScanNetwork() {
  if (!manager_proxy_)
    return;

  dleyna_manager_call_get_renderers(
      manager_proxy_,
      NULL,
      OnScanNetworkCallBack,
      this);
}

void MediaRendererManager::GetRenderers(const picojson::value& value) {
  if (!manager_proxy_)
    return;

  double async_call_id = value.get("asyncCallId").get<double>();

  dleyna_manager_call_get_renderers(
      manager_proxy_,
      cancellable_,
      OnGetRenderersCallBack,
      new CallbackData(this, async_call_id));
}

void MediaRendererManager::HandleOpenURI(const picojson::value& value) {
  if (MediaRendererPtr renderer = GetMediaRendererById(value))
    renderer->OpenURI(value);
}

void MediaRendererManager::HandlePrefetchURI(
    const picojson::value& value) {
  if (MediaRendererPtr renderer = GetMediaRendererById(value))
    renderer->PrefetchURI(value);
}

void MediaRendererManager::HandleCancel(const picojson::value& value) {
  if (MediaRendererPtr renderer = GetMediaRendererById(value))
    renderer->Cancel(value);
}

void MediaRendererManager::HandlePlay(const picojson::value& value) {
  if (MediaRendererPtr renderer = GetMediaRendererById(value))
    renderer->Play(value);
}

void MediaRendererManager::HandlePause(const picojson::value& value) {
  if (MediaRendererPtr renderer = GetMediaRendererById(value))
    renderer->Pause(value);
}

void MediaRendererManager::HandleStop(const picojson::value& value) {
  if (MediaRendererPtr renderer = GetMediaRendererById(value))
    renderer->Stop(value);
}

void MediaRendererManager::HandleNext(const picojson::value& value) {
  if (MediaRendererPtr renderer = GetMediaRendererById(value))
    renderer->Next(value);
}

void MediaRendererManager::HandlePrevious(const picojson::value& value) {
  if (MediaRendererPtr renderer = GetMediaRendererById(value))
    renderer->Previous(value);
}

void MediaRendererManager::HandleMute(const picojson::value& value) {
  if (MediaRendererPtr renderer = GetMediaRendererById(value))
    renderer->Mute(value);
}

void MediaRendererManager::HandleSetSpeed(const picojson::value& value) {
  if (MediaRendererPtr renderer = GetMediaRendererById(value))
    renderer->SetSpeed(value);
}

void MediaRendererManager::HandleSetVolume(const picojson::value& value) {
  if (MediaRendererPtr renderer = GetMediaRendererById(value))
    renderer->SetVolume(value);
}

void MediaRendererManager::HandleGotoTrack(const picojson::value& value) {
  if (MediaRendererPtr renderer = GetMediaRendererById(value))
    renderer->GotoTrack(value);
}

void MediaRendererManager::PostRendererFound(const std::string& path) {
  MediaRendererPtr media_renderer(new MediaRenderer(instance_, path));
  media_renderers_.insert(MediaRendererPair(path, media_renderer));
  picojson::value::object object;
  object["cmd"] = picojson::value("rendererFound");
  object["renderer"] = media_renderer->ToJSON();
  picojson::value value(object);
  instance_->PostMessage(value.serialize().c_str());
}


MediaRendererPtr MediaRendererManager::GetMediaRendererById(
    const picojson::value& id) {
  return GetMediaRendererById(id.get("rendererId").to_str());
}

MediaRendererPtr MediaRendererManager::GetMediaRendererById(
    const std::string& id) {
  if (!media_renderers_.size())
    return MediaRendererPtr();

  std::map<std::string, MediaRendererPtr>::const_iterator it;
  if ((it = media_renderers_.find(id)) != media_renderers_.end())
    return (*it).second;

  return MediaRendererPtr();
}

void MediaRendererManager::OnScanNetwork(
    GObject* source_object,
    GAsyncResult* res) {
  GError* gerror = NULL;
  gchar** out_renderers;
  if (!dleyna_manager_call_get_renderers_finish(
      manager_proxy_,
      &out_renderers,
      res,
      &gerror)) {
    g_error_free(gerror);
    return;
  }

  while (gchar* renderer_path = *out_renderers) {
    PostRendererFound(std::string(*out_renderers));
    out_renderers++;
    g_free(renderer_path);
  }
}

void MediaRendererManager::OnGetRenderers(
    GObject* source_object,
    GAsyncResult* res,
    double async_id) {
  GError* gerror = NULL;
  gchar** out_renderers;
  if (!dleyna_manager_call_get_renderers_finish(
      manager_proxy_,
      &out_renderers,
      res,
      &gerror)) {
    g_error_free(gerror);
    return;
  }

  picojson::value::array renderers;

  while (gchar* renderer_path = *out_renderers) {
    MediaRendererPtr media_renderer(new MediaRenderer(instance_,
        std::string(*out_renderers)));
    media_renderers_.insert(MediaRendererPair(std::string(*out_renderers),
        media_renderer));
    renderers.push_back(media_renderer->ToJSON());
    out_renderers++;
    g_free(renderer_path);
  }

  picojson::value::object object;
  object["cmd"] = picojson::value("getRenderersCompleted");
  object["asyncCallId"] = picojson::value(async_id);
  object["renderers"] =   picojson::value(renderers);
  picojson::value value(object);
  instance_->PostMessage(value.serialize().c_str());
}

void MediaRendererManager::OnFoundRenderer(dleynaManager* object,
                                           const gchar* arg_Path) {
  PostRendererFound(std::string(arg_Path));
}

void MediaRendererManager::OnLostRenderer(dleynaManager* object,
                                          const gchar* arg_Path) {
  MediaRendererPtr renderer = GetMediaRendererById(std::string(arg_Path));
  if (!renderer)
    return;

  picojson::value::object pobject;
  pobject["cmd"] = picojson::value("rendererLost");
  pobject["rendererId"] = renderer->ToJSON().get("id");
  picojson::value value(pobject);
  instance_->PostMessage(value.serialize().c_str());
  media_renderers_.erase(arg_Path);
}

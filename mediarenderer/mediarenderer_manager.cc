// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mediarenderer/mediarenderer_manager.h"

#include <utility>
#include <string>
#include "common/extension.h"
#include "mediarenderer/mediarenderer.h"

typedef std::pair<std::string, std::shared_ptr<MediaRenderer>> MediaRendererPair;

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

void MediaRendererManager::scanNetwork() {
  if (!manager_proxy_)
    return;

  dleyna_manager_call_get_renderers(
      manager_proxy_,
      NULL,
      OnScanNetworkCallBack,
      this);
}

void MediaRendererManager::getRenderers(const picojson::value& value) {
  if (!manager_proxy_)
    return;

  double async_call_id = value.get("asyncCallId").get<double>();

  dleyna_manager_call_get_renderers(
      manager_proxy_,
      cancellable_,
      OnGetRenderersCallBack,
      new CallbackData(this, async_call_id));
}

void MediaRendererManager::handleOpenURI(const picojson::value& value) {
  if (MediaRendererPtr renderer = getMediaRendererById(value))
    renderer->openURI(value);
}

void MediaRendererManager::handlePrefetchURI(
    const picojson::value& value) {
  if (MediaRendererPtr renderer = getMediaRendererById(value))
    renderer->prefetchURI(value);
}

void MediaRendererManager::handleCancel(const picojson::value& value) {
  if (MediaRendererPtr renderer = getMediaRendererById(value))
    renderer->cancel(value);
}

void MediaRendererManager::handlePlay(const picojson::value& value) {
  if (MediaRendererPtr renderer = getMediaRendererById(value))
    renderer->play(value);
  else
    fprintf(stderr, "oopd\n");
}

void MediaRendererManager::handlePause(const picojson::value& value) {
  if (MediaRendererPtr renderer = getMediaRendererById(value))
    renderer->pause(value);
}

void MediaRendererManager::handleStop(const picojson::value& value) {
  if (MediaRendererPtr renderer = getMediaRendererById(value))
    renderer->stop(value);
}

void MediaRendererManager::handleNext(const picojson::value& value) {
  if (MediaRendererPtr renderer = getMediaRendererById(value))
    renderer->next(value);
}

void MediaRendererManager::handlePrevious(const picojson::value& value) {
  if (MediaRendererPtr renderer = getMediaRendererById(value))
    renderer->previous(value);
}

void MediaRendererManager::handleMute(const picojson::value& value) {
  if (MediaRendererPtr renderer = getMediaRendererById(value))
    renderer->mute(value);
}

void MediaRendererManager::handleSetSpeed(const picojson::value& value) {
  if (MediaRendererPtr renderer = getMediaRendererById(value))
    renderer->setSpeed(value);
}

void MediaRendererManager::handleSetVolume(const picojson::value& value) {
  if (MediaRendererPtr renderer = getMediaRendererById(value))
    renderer->setVolume(value);
}

void MediaRendererManager::handleGotoTrack(const picojson::value& value) {
  if (MediaRendererPtr renderer = getMediaRendererById(value))
    renderer->gotoTrack(value);
}

void MediaRendererManager::postRendererFound(const std::string& path) {
 MediaRendererPtr media_renderer(new MediaRenderer(instance_, path));
 fprintf(stderr, "Renderer FOUND: %s\n", path.c_str());
  media_renderers_.insert(MediaRendererPair(path, media_renderer));
  picojson::value::object object;
  object["cmd"] = picojson::value("rendererFound");
 object["renderer"] = media_renderer->toJSON();
 picojson::value value(object);
 instance_->PostMessage(value.serialize().c_str());
}


MediaRendererPtr MediaRendererManager::getMediaRendererById(
    const picojson::value& id) {
  return getMediaRendererById(id.get("rendererId").to_str());
}

MediaRendererPtr MediaRendererManager::getMediaRendererById(const std::string& id) {
  if (media_renderers_.size()) {
    std::map<std::string, MediaRendererPtr>::const_iterator it;
    fprintf(stderr, "Searching Renderer: %s\n", id.c_str());
    if ((it = media_renderers_.find(id)) != media_renderers_.end())
      return (*it).second;
  }
  return MediaRendererPtr();
}

void MediaRendererManager::OnScanNetwork(
    GObject *source_object,
    GAsyncResult *res) {
  GError* gerror = NULL;
  gchar **out_renderers;
  if (!dleyna_manager_call_get_renderers_finish(
      manager_proxy_,
      &out_renderers,
      res,
      &gerror)) {
    g_error_free(gerror);
    return;
  }

  while (gchar* renderer_path = *out_renderers) {
    postRendererFound(std::string(*out_renderers));
    out_renderers++;
    g_free(renderer_path);
  }
}

void MediaRendererManager::OnGetRenderers(
    GObject *source_object,
    GAsyncResult *res,
    double async_id) {
  GError* gerror = NULL;
  gchar **out_renderers;
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
    fprintf(stderr, "GetRenderers: ID: %s\n", renderer_path);
    media_renderers_.insert(MediaRendererPair(std::string(*out_renderers),
        media_renderer));
    renderers.push_back(media_renderer->toJSON());
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

void MediaRendererManager::OnFoundRenderer(
    dleynaManager *object,
    const gchar *arg_Path) {
  postRendererFound(std::string(arg_Path));
}

void MediaRendererManager::OnLostRenderer(
    dleynaManager *object,
    const gchar *arg_Path) {
  if (MediaRendererPtr renderer = getMediaRendererById(std::string(arg_Path))) {
      picojson::value::object object;
      object["cmd"] = picojson::value("rendererLost");
      object["rendererId"] = renderer->toJSON().get("id");
      picojson::value value(object);
      instance_->PostMessage(value.serialize().c_str());
      media_renderers_.erase(arg_Path);
      fprintf(stderr, "Lost Renderer: %s\n", arg_Path);
  }
}

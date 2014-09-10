// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media_renderer/media_renderer.h"

#include <map>
#include "common/extension.h"

namespace {

const gchar kdLeynaRendererInterfaceName[] = "com.intel.dleyna-renderer";
const gchar kdLeynaPushHostInterfaceName[] = "com.intel.dleyna-renderer";
const gchar kMprisPlayerInterfaceName[] = "com.intel.dleyna-renderer";

}  // namespace

static picojson::value toJSONValue(const gchar* value) {
  return value ? picojson::value(value) : picojson::value();
}

static picojson::value toJSONValueArray(GVariant* values) {
  picojson::array array;
  GVariantIter iter;
  GVariant* child;
  gdouble value;

  if (!values)
    return picojson::value(array);

  g_variant_iter_init(&iter, values);
  while (g_variant_iter_next(&iter, "d", &value))
    array.push_back(picojson::value(value));

  return picojson::value(array);
}

void
MediaRenderer::PropertyChanged(GDBusProxy* proxy,
    GVariant* changed_properties,
    GStrv invalidated_properties,
    gpointer user_data) {
}

MediaRenderer::MediaRenderer(common::Instance* instance,
                             const std::string& object_path)
    : instance_(instance),
      rendererdevice_proxy_(0),
      mediaplayer2_proxy_(0),
      mprisplayer_proxy_(0),
      object_path_(object_path),
      cancellable_(g_cancellable_new()) {
  GError* gerror = NULL;

  rendererdevice_proxy_ = dleyna_renderer_device_proxy_new_for_bus_sync(
      G_BUS_TYPE_SESSION,
      G_DBUS_PROXY_FLAGS_NONE,
      kdLeynaRendererInterfaceName,
      object_path.c_str(),
      NULL,
      &gerror);

  if (gerror) {
    g_error_free(gerror);
    return;
  }

  pushhost_proxy_ = dleyna_push_host_proxy_new_for_bus_sync(
      G_BUS_TYPE_SESSION,
      G_DBUS_PROXY_FLAGS_NONE,
      kdLeynaPushHostInterfaceName,
      object_path.c_str(),
      NULL,
      &gerror);

  if (gerror) {
    g_error_free(gerror);
    return;
  }

  mprisplayer_proxy_ = mprismediaplayer2_player_proxy_new_for_bus_sync(
      G_BUS_TYPE_SESSION,
      G_DBUS_PROXY_FLAGS_NONE,
      kMprisPlayerInterfaceName,
      object_path.c_str(),
      NULL,
      &gerror);

  g_signal_connect(
      mprisplayer_proxy_,
      "g-properties-changed",
      G_CALLBACK(MediaRenderer::PropertyChanged),
      this);

  if (gerror) {
    g_error_free(gerror);
    return;
  }
}

MediaRenderer::~MediaRenderer() {
  g_object_unref(rendererdevice_proxy_);
  g_object_unref(pushhost_proxy_);
  g_object_unref(mprisplayer_proxy_);
}

picojson::value MediaRenderer::ToJSON() {
  if (object_.size())
    return picojson::value(object_);

  object_["id"] = picojson::value(object_path_);
  object_["friendlyName"] = toJSONValue(
      dleyna_renderer_device_get_friendly_name(rendererdevice_proxy_));
  object_["manufacturer"] = toJSONValue(
      dleyna_renderer_device_get_manufacturer(rendererdevice_proxy_));
  object_["manufacturerURL"] = toJSONValue(
      dleyna_renderer_device_get_manufacturer_url(rendererdevice_proxy_));
  object_["modelDescription"] = toJSONValue(
      dleyna_renderer_device_get_model_description(rendererdevice_proxy_));
  object_["modelName"] = toJSONValue(
      dleyna_renderer_device_get_model_name(rendererdevice_proxy_));
  object_["modelNumber"] = toJSONValue(
      dleyna_renderer_device_get_model_number(rendererdevice_proxy_));
  object_["serialNumber"] = toJSONValue(
      dleyna_renderer_device_get_serial_number(rendererdevice_proxy_));
  object_["UDN"] = toJSONValue(
      dleyna_renderer_device_get_udn(rendererdevice_proxy_));
  object_["presentationURL"] = toJSONValue(
      dleyna_renderer_device_get_presentation_url(rendererdevice_proxy_));
  object_["iconURL"] = toJSONValue(
      dleyna_renderer_device_get_icon_url(rendererdevice_proxy_));
  object_["deviceType"] = toJSONValue(
      dleyna_renderer_device_get_device_type(rendererdevice_proxy_));
  object_["protocolInfo"] = toJSONValue(
      dleyna_renderer_device_get_protocol_info(rendererdevice_proxy_));

  picojson::value::object controller_object;
  controller_object["id"] = picojson::value(object_path_);
  controller_object["playbackStatus"] = toJSONValue(
      mprismediaplayer2_player_get_playback_status(mprisplayer_proxy_));
  controller_object["muted"] = picojson::value(static_cast<bool>
     (mprismediaplayer2_player_get_mute(mprisplayer_proxy_)));
  controller_object["volume"] = picojson::value(
      mprismediaplayer2_player_get_volume(mprisplayer_proxy_));
  controller_object["track"] = picojson::value(static_cast<double>
      (mprismediaplayer2_player_get_current_track(mprisplayer_proxy_)));
  controller_object["speed"] = picojson::value(
      mprismediaplayer2_player_get_rate(mprisplayer_proxy_));
  controller_object["playSpeeds"] = toJSONValueArray(
      mprismediaplayer2_player_get_transport_play_speeds(
          mprisplayer_proxy_));
  object_["controller"] = picojson::value(controller_object);

  return picojson::value(object_);
}

// MediaRenderer methods
void MediaRenderer::OpenURI(const picojson::value& value) {
  double async_call_id = value.get("asyncCallId").get<double>();

  if (!mprisplayer_proxy_) {
    PostError(async_call_id);
    return;
  }

  if (!pushhost_proxy_) {
    PostError(async_call_id);
    return;
  }

  dleyna_push_host_call_host_file(
      pushhost_proxy_,
      value.get("mediaURI").to_str().c_str(),
      cancellable_,
      OnHostFileCallBack,
      new CallbackData(this, async_call_id));
}

void MediaRenderer::PrefetchURI(const picojson::value& value) {
  double async_call_id = value.get("asyncCallId").get<double>();

  if (!mprisplayer_proxy_) {
    PostError(async_call_id);
    return;
  }

  mprismediaplayer2_player_call_open_next_uri(
      mprisplayer_proxy_,
      value.get("mediaURI").to_str().c_str(),
      value.get("metaData").to_str().c_str(),
      cancellable_,
      OnPrefetchURICallBack,
      new CallbackData(this, async_call_id));
}

void MediaRenderer::Cancel(const picojson::value& value) {
  double async_call_id = value.get("asyncCallId").get<double>();

  if (!rendererdevice_proxy_) {
    PostError(async_call_id);
    return;
  }

  dleyna_renderer_device_call_cancel(
      rendererdevice_proxy_,
      cancellable_,
      OnCancelCallBack,
      new CallbackData(this, async_call_id));
}

bool MediaRenderer::IsCancelled() const {
  return g_cancellable_is_cancelled(cancellable_);
}

// MediaController methods
void MediaRenderer::Play(const picojson::value& value) {
  double async_call_id = value.get("asyncCallId").get<double>();

  if (!mprisplayer_proxy_) {
    PostError(async_call_id);
    return;
  }

  mprismediaplayer2_player_call_play(
      mprisplayer_proxy_,
      cancellable_,
      OnPlayCallBack,
      new CallbackData(this, async_call_id));
}

void MediaRenderer::Pause(const picojson::value& value) {
  double async_call_id = value.get("asyncCallId").get<double>();

  if (!mprisplayer_proxy_) {
    PostError(async_call_id);
    return;
  }

  mprismediaplayer2_player_call_pause(
      mprisplayer_proxy_,
      cancellable_,
      OnPauseCallBack,
      new CallbackData(this, async_call_id));
}

void MediaRenderer::Stop(const picojson::value& value) {
  double async_call_id = value.get("asyncCallId").get<double>();

  if (!mprisplayer_proxy_) {
    PostError(async_call_id);
    return;
  }

  mprismediaplayer2_player_call_stop(
      mprisplayer_proxy_,
      cancellable_,
      OnStopCallBack,
      new CallbackData(this, async_call_id));
}

void MediaRenderer::Next(const picojson::value& value) {
  double async_call_id = value.get("asyncCallId").get<double>();

  if (!mprisplayer_proxy_) {
    PostError(async_call_id);
    return;
  }

  mprismediaplayer2_player_call_next(
      mprisplayer_proxy_,
      cancellable_,
      OnNextCallBack,
      new CallbackData(this, async_call_id));
}

void MediaRenderer::Previous(const picojson::value& value) {
  double async_call_id = value.get("asyncCallId").get<double>();

  if (!mprisplayer_proxy_) {
    PostError(async_call_id);
    return;
  }

  mprismediaplayer2_player_call_previous(
      mprisplayer_proxy_,
      cancellable_,
      OnPreviousCallBack,
      new CallbackData(this, async_call_id));
}

void MediaRenderer::Mute(const picojson::value& value) {
  double async_call_id = value.get("asyncCallId").get<double>();

  if (!mprisplayer_proxy_) {
    PostError(async_call_id);
    return;
  }

  mprismediaplayer2_player_set_mute(
      mprisplayer_proxy_,
      (gboolean)value.get("mute").get<bool>());
  PostResult("setMuteCompleted", value.get("asyncCallId").get<double>());
}

void MediaRenderer::SetSpeed(const picojson::value& value) {
  double async_call_id = value.get("asyncCallId").get<double>();

  if (!mprisplayer_proxy_) {
    PostError(async_call_id);
    return;
  }

  mprismediaplayer2_player_set_rate(
      mprisplayer_proxy_,
      value.get("speed").get<double>());
  PostResult("setSpeedCompleted", value.get("asyncCallId").get<double>());
}

void MediaRenderer::SetVolume(const picojson::value& value) {
  double async_call_id = value.get("asyncCallId").get<double>();

  if (!mprisplayer_proxy_) {
    PostError(async_call_id);
    return;
  }
  mprismediaplayer2_player_set_volume(
      mprisplayer_proxy_,
      value.get("volume").get<double>());
  PostResult("setVolumeCompleted", value.get("asyncCallId").get<double>());
}

void MediaRenderer::GotoTrack(const picojson::value& value) {
  double async_call_id = value.get("asyncCallId").get<double>();

  if (!mprisplayer_proxy_) {
    PostError(async_call_id);
    return;
  }

  mprismediaplayer2_player_call_goto_track(
      mprisplayer_proxy_,
      (guint) value.get("track").get<double>(),
      cancellable_,
      OnGoToTrackCallBack,
      new CallbackData(this, async_call_id));
}

void MediaRenderer::PostResult(
    const std::string& completed_operation,
    double async_operation_id) {
  picojson::value::object object;
  object["cmd"] = picojson::value(completed_operation);
  object["asyncCallId"] = picojson::value(async_operation_id);
  picojson::value value(object);
  instance_->PostMessage(value.serialize().c_str());
}

void MediaRenderer::PostError(double async_operation_id) {
  picojson::value::object object;
  object["cmd"] = picojson::value("asyncCallError");
  object["asyncCallId"] = picojson::value(async_operation_id);
  picojson::value value(object);
  instance_->PostMessage(value.serialize().c_str());
}

void MediaRenderer::OnOpenURI(
    GObject* source_object,
    GAsyncResult* res,
    double async_id) {
  GError* gerror = NULL;

  if (mprismediaplayer2_player_call_open_uri_ex_finish(
      reinterpret_cast<mprismediaplayer2Player*>(source_object),
      res,
      &gerror))
    PostResult("openURICompleted", async_id);
  else
    PostError(async_id);

  if (gerror)
    g_error_free(gerror);
}

void MediaRenderer::OnHostFile(
    GObject* source_object,
    GAsyncResult* res,
    double async_id) {
  GError* gerror = NULL;
  gchar* uploaded_path = NULL;

  if (dleyna_push_host_call_host_file_finish(
      reinterpret_cast<dleynaPushHost*>(source_object),
      &uploaded_path,
      res,
      &gerror)) {
    mprismediaplayer2_player_call_open_uri_ex(
    mprisplayer_proxy_,
    uploaded_path,
    uploaded_path,
    cancellable_,
    OnOpenURICallBack,
    new CallbackData(this, async_id));
  } else {
    PostError(async_id);
  }

  if (gerror)
    g_error_free(gerror);
}

void MediaRenderer::OnPrefetchURI(
    GObject* source_object,
    GAsyncResult* res,
    double async_id) {
  GError* gerror = NULL;
  GVariant* objects;
  guint totalItems;

  if (mprismediaplayer2_player_call_open_next_uri_finish(
      reinterpret_cast<mprismediaplayer2Player*>(source_object),
      res,
      &gerror))
    PostResult("prefetchURICompleted", async_id);
  else
    PostError(async_id);

  if (gerror)
    g_error_free(gerror);
}

void MediaRenderer::OnCancel(
    GObject* source_object,
    GAsyncResult* res,
    double async_id) {
  GError* gerror = NULL;
  gchar** out_Paths = NULL;

  if (dleyna_renderer_device_call_cancel_finish(
      reinterpret_cast<dleynaRendererDevice*>(source_object),
      res,
      &gerror))
    PostResult("cancelCompleted", async_id);
  else
    PostError(async_id);

  if (gerror)
    g_error_free(gerror);
}

void MediaRenderer::OnPlay(
    GObject* source_object,
    GAsyncResult* res,
    double async_id) {
  GError* gerror = NULL;
  gchar** out_Paths = NULL;

  if (mprismediaplayer2_player_call_play_finish(
      reinterpret_cast<mprismediaplayer2Player*>(source_object),
      res,
      &gerror))
    PostResult("playCompleted", async_id);
  else
    PostError(async_id);

  if (gerror)
    g_error_free(gerror);
}

void MediaRenderer::OnPause(
    GObject* source_object,
    GAsyncResult* res,
    double async_id) {
  GError* gerror = NULL;
  gchar** out_Paths = NULL;

  if (mprismediaplayer2_player_call_pause_finish(
      reinterpret_cast<mprismediaplayer2Player*>(source_object),
      res,
      &gerror))
    PostResult("pauseCompleted", async_id);
  else
    PostError(async_id);

  if (gerror)
    g_error_free(gerror);
}

void MediaRenderer::OnStop(
    GObject* source_object,
    GAsyncResult* res,
    double async_id) {
  GError* gerror = NULL;
  gchar** out_Paths = NULL;

  if (mprismediaplayer2_player_call_stop_finish(
      reinterpret_cast<mprismediaplayer2Player*>(source_object),
      res,
      &gerror))
    PostResult("stopCompleted", async_id);
  else
    PostError(async_id);

  if (gerror)
    g_error_free(gerror);
}

void MediaRenderer::OnNext(
    GObject* source_object,
    GAsyncResult* res,
    double async_id) {
  GError* gerror = NULL;
  gchar** out_Paths = NULL;

  if (mprismediaplayer2_player_call_next_finish(
      reinterpret_cast<mprismediaplayer2Player*>(source_object),
      res,
      &gerror))
    PostResult("nextCompleted", async_id);
  else
    PostError(async_id);

  if (gerror)
    g_error_free(gerror);
}

void MediaRenderer::OnPrevious(
    GObject* source_object,
    GAsyncResult* res,
    double async_id) {
  GError* gerror = NULL;
  gchar** out_Paths = NULL;

  if (mprismediaplayer2_player_call_previous_finish(
      reinterpret_cast<mprismediaplayer2Player*>(source_object),
      res,
      &gerror))
    PostResult("previousCompleted", async_id);
  else
    PostError(async_id);

  if (gerror)
    g_error_free(gerror);
}

void MediaRenderer::OnGoToTrack(
    GObject* source_object,
    GAsyncResult* res,
    double async_id) {
  GError* gerror = NULL;
  gchar** out_Paths = NULL;

  if (mprismediaplayer2_player_call_goto_track_finish(
      reinterpret_cast<mprismediaplayer2Player*>(source_object),
      res,
      &gerror))
    PostResult("gotoTrackCompleted", async_id);
  else
    PostError(async_id);

  if (gerror)
    g_error_free(gerror);
}

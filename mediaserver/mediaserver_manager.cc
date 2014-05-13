// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mediaserver/mediaserver_manager.h"

#include <utility>
#include <string>
#include "common/extension.h"
#include "mediaserver/mediaserver.h"

typedef std::pair<std::string, std::shared_ptr<MediaServer>> MediaServerPair;

MediaServerManager::MediaServerManager(common::Instance* instance)
    : instance_(instance) {
  GError* gerror = NULL;
  manager_proxy_ = dleyna_manager_proxy_new_for_bus_sync(
      G_BUS_TYPE_SESSION,
      G_DBUS_PROXY_FLAGS_NONE,
      "com.intel.dleyna-server",
      "/com/intel/dLeynaServer",
      NULL,
      &gerror);

  if (gerror) {
    g_error_free(gerror);
    return;
  }

  g_signal_connect(
      manager_proxy_,
      "found-server",
      G_CALLBACK(OnFoundServerCallBack),
      this);

  g_signal_connect(
      manager_proxy_,
      "lost-server",
      G_CALLBACK(OnLostServerCallBack),
      this);
}

MediaServerManager::~MediaServerManager() {
  g_object_unref(manager_proxy_);
}

void MediaServerManager::scanNetwork() {
  if (!manager_proxy_)
    return;

  dleyna_manager_call_get_servers(
      manager_proxy_,
      NULL,
      OnScanNetworkCallBack,
      this);
}

void MediaServerManager::getServers(const picojson::value& value) {
  if (!manager_proxy_)
    return;

  double async_call_id = value.get("asyncCallId").get<double>();

  dleyna_manager_call_get_servers(
      manager_proxy_,
      cancellable_,
      OnGetServersCallBack,
      new CallbackData(this, async_call_id));
}

void MediaServerManager::handleBrowse(const picojson::value& value) {
  if (MediaServerPtr server = getMediaServerById(value))
    server->browse(value);
}

void MediaServerManager::handleFind(const picojson::value& value) {
  if (MediaServerPtr server = getMediaServerById(value))
    server->find(value);
}

void MediaServerManager::handleCreateFolder(const picojson::value& value) {
  if (MediaServerPtr server = getMediaServerById(value))
    server->createFolder(value);
}

void MediaServerManager::handleUpload(const picojson::value& value) {
  if (MediaServerPtr server = getMediaServerById(value))
    server->upload(value);
}

void MediaServerManager::handleCancel(const picojson::value& value) {
  if (MediaServerPtr server = getMediaServerById(value))
    server->cancel(value);
}

void MediaServerManager::handleUploadToContainer(const picojson::value& value) {
  if (MediaServerPtr server =
      getMediaServerById(value.get("rootContainerId").to_str()))
    server->uploadToContainer(value);
}

void MediaServerManager::handleCreateFolderInContainer(
    const picojson::value& value) {
  if (MediaServerPtr server =
      getMediaServerById(value.get("rootContainerId").to_str()))
    server->createFolderInContainer(value);
}


void MediaServerManager::postServerFound(const std::string& path) {
  MediaServerPtr media_server(new MediaServer(instance_, path));
  media_servers_.insert(MediaServerPair(path, media_server));
  picojson::value::object object;
  object["cmd"] = picojson::value("serverFound");
  object["server"] = media_server->toJSON();
  picojson::value value(object);
  instance_->PostMessage(value.serialize().c_str());
}

MediaServerPtr MediaServerManager::getMediaServerById(
    const picojson::value& id) {
  return getMediaServerById(id.get("serverId").to_str());
}

MediaServerPtr MediaServerManager::getMediaServerById(const std::string& id) {
  if (media_servers_.size()) {
    std::map<std::string, MediaServerPtr>::const_iterator it;
    if ((it = media_servers_.find(id)) != media_servers_.end())
      return (*it).second;
  }
  return MediaServerPtr();
}

void MediaServerManager::OnScanNetwork(
    GObject *source_object,
    GAsyncResult *res) {
  GError* gerror = NULL;
  gchar **out_servers;
  if (!dleyna_manager_call_get_servers_finish(
      manager_proxy_,
      &out_servers,
      res,
      &gerror)) {
    g_error_free(gerror);
    return;
  }

  while (gchar* server_path = *out_servers) {
    postServerFound(std::string(*out_servers));
    out_servers++;
    g_free(server_path);
  }
}

void MediaServerManager::OnGetServers(
    GObject *source_object,
    GAsyncResult *res,
    double async_id) {
  GError* gerror = NULL;
  gchar **out_servers;
  if (!dleyna_manager_call_get_servers_finish(
      manager_proxy_,
      &out_servers,
      res,
      &gerror)) {
    g_error_free(gerror);
    return;
  }

  picojson::value::array servers;

  while (gchar* server_path = *out_servers) {
    MediaServerPtr media_server(new MediaServer(instance_,
        std::string(*out_servers)));
    media_servers_.insert(MediaServerPair(std::string(*out_servers),
        media_server));
    servers.push_back(media_server->toJSON());
    out_servers++;
    g_free(server_path);
  }

  picojson::value::object object;
  object["cmd"] = picojson::value("getServersCompleted");
  object["asyncCallId"] = picojson::value(async_id);
  object["servers"] =   picojson::value(servers);
  picojson::value value(object);
  instance_->PostMessage(value.serialize().c_str());
}

void MediaServerManager::OnFoundServer(
    dleynaManager *object,
    const gchar *arg_Path) {
  postServerFound(std::string(arg_Path));
}

void MediaServerManager::OnLostServer(
    dleynaManager *object,
    const gchar *arg_Path) {
  if (MediaServerPtr server = getMediaServerById(std::string(arg_Path))) {
      picojson::value::object object;
      object["cmd"] = picojson::value("serverLost");
      object["serverId"] = server->toJSON().get("id");
      picojson::value value(object);
      instance_->PostMessage(value.serialize().c_str());
      media_servers_.erase(arg_Path);
  }
}

// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mediaserver/mediaserver_instance.h"

#include <string>

#include "common/picojson.h"
#include "mediaserver/mediaserver_manager.h"

MediaServerInstance::MediaServerInstance()
    : worker_thread_(&MediaServerInstance::InitWorkerThread, this) {
  worker_thread_.detach();
}

MediaServerInstance::~MediaServerInstance() {
  g_main_loop_quit(worker_loop_);
  delete media_server_manager_;
}

gboolean MediaServerInstance::CreateMediaServerManager(void* data) {
  if (!data) {
    std::cerr << "Null pointer is passed to callback" << std::endl;
    return FALSE;
  }

  MediaServerInstance* instance = static_cast<MediaServerInstance*>(data);
  instance->media_server_manager_ = new MediaServerManager(instance);
  return FALSE;
}

void MediaServerInstance::InitWorkerThread() {
  GMainContext* context = g_main_context_default();
  worker_loop_ = g_main_loop_new(context, FALSE);
  g_main_context_push_thread_default(context);
  GSource* source = g_idle_source_new();
  g_source_set_callback(source,
                        &MediaServerInstance::CreateMediaServerManager,
                        this,
                        NULL);
  g_source_attach(source, context);
  g_main_loop_run(worker_loop_);
  g_source_destroy(source);
  g_source_unref(source);
  g_main_loop_unref(worker_loop_);
}

void MediaServerInstance::HandleMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "scanNetwork")
    media_server_manager_->scanNetwork();
  else if (cmd == "browse")
    media_server_manager_->handleBrowse(v);
  else if (cmd == "find")
    media_server_manager_->handleFind(v);
  else if (cmd == "createFolder")
    media_server_manager_->handleCreateFolder(v);
  else if (cmd == "upload")
    media_server_manager_->handleUpload(v);
  else if (cmd == "cancel")
    media_server_manager_->handleCancel(v);
  else if (cmd == "uploadToContainer")
    media_server_manager_->handleUploadToContainer(v);
  else if (cmd == "createFolderInContainer")
    media_server_manager_->handleCreateFolderInContainer(v);
  else
    std::cerr << "Received unknown message: " << cmd << "\n";
}

void MediaServerInstance::HandleSyncMessage(const char* message) {}

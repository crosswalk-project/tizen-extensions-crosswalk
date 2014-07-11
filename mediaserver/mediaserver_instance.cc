// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mediaserver/mediaserver_instance.h"

#include <string>

#include "common/picojson.h"
#include "mediaserver/mediaserver_manager.h"

MediaServerInstance::MediaServerInstance() {
  media_server_manager_ = new MediaServerManager(this);
}

MediaServerInstance::~MediaServerInstance() {
  delete media_server_manager_;
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
  else if (cmd == "getServers")
    media_server_manager_->getServers(v);
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

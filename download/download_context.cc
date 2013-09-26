// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "download/download_context.h"
#include "download/download_utils.h"

#include "common/picojson.h"

DEFINE_XWALK_EXTENSION(DownloadContext);

using download_utils::EnumToPChar;
using download_utils::ToString;

#define CHECK(x, args) do { \
  int retval = (x); \
  if (retval != DOWNLOAD_ERROR_NONE) { \
    fprintf(stderr, "Download error: %s returned %s at %s:%d \n", #x, \
                    EnumToPChar(retval), __FILE__, __LINE__); \
    OnFailedInfo(args, ToString(EnumToPChar(retval))); \
    return; \
  } \
} while (0)

#define CHECK_DO(x, args, y) do { \
  int retval = (x); \
  if (retval != DOWNLOAD_ERROR_NONE) { \
    fprintf(stderr, "Download error: %s returned %s at %s:%d \n", #x, \
                    EnumToPChar(retval), __FILE__, __LINE__); \
    OnFailedInfo(args, ToString(EnumToPChar(retval))); \
    y; \
  } \
} while (0)

DownloadContext::DownloadContext(ContextAPI* api)
    : api_(api) {
}

DownloadContext::~DownloadContext() {
  delete (api_);
  for (DownloadArgsVector::iterator it = args_.begin();
       it != args_.end(); it++) {
    delete (*it);
  }
}

const char DownloadContext::name[] = "tizen.download";

// This will be generated from download_api.js.
extern const char kSource_download_api[];

const char* DownloadContext::GetJavaScript() {
  return kSource_download_api;
}

void DownloadContext::HandleMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    fprintf(stderr, "Ignoring message.\n");
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "DownloadStart")
    HandleStart(v);
  else if (cmd == "DownloadPause")
    HandleGeneral(v, download_pause, "HandlePause");
  else if (cmd == "DownloadResume")
    HandleGeneral(v, download_start, "HandleResume");
  else if (cmd == "DownloadCancel")
    HandleGeneral(v, download_cancel, "HandleCancel");
  else if (cmd == "DownloadGetNetworkType")
    HandleGetNetworkType(v);
  else
    fprintf(stderr, "Not supported async command %s\n", cmd.c_str());
}

void DownloadContext::HandleSyncMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    fprintf(stderr, "Ignoring message.\n");
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "DownloadGetState")
    HandleGetState(v);
  else if (cmd == "DownloadGetMIMEType")
    HandleGetMIMEType(v);
  else
    fprintf(stderr, "Not supported sync command %s\n", cmd.c_str());
}

void DownloadContext::OnStateChanged(int download_id,
                                     download_state_e state, void* user_data) {
  DownloadArgs* args = static_cast<DownloadArgs*>(user_data);
  switch (state) {
  case DOWNLOAD_STATE_DOWNLOADING:
    OnStartInfo(download_id, user_data);
    break;
  case DOWNLOAD_STATE_PAUSED:
    OnPausedInfo(user_data);
    break;
  case DOWNLOAD_STATE_COMPLETED:
    OnFinishedInfo(download_id, user_data);
    break;
  case DOWNLOAD_STATE_CANCELED:
    OnCanceledInfo(user_data);
    break;
  case DOWNLOAD_STATE_FAILED: {
      download_error_e error;
      CHECK(download_get_error(download_id, &error), args);
      args->context->OnFailedInfo(args, ToString(EnumToPChar(error)));
    }
    break;
  default:
    fprintf(stderr, "Ignore state: %d\n", state);
    break;
  }
}

void DownloadContext::OnStartInfo(int download_id, void* user_param) {
  DownloadArgs* args = static_cast<DownloadArgs*>(user_param);
  const std::string uid(args->download_uid);
  DownloadItemRefPtr downloadItem = args->context->downloads_[uid];

  long long unsigned file_size = 0;
  CHECK(download_get_content_size(download_id, &file_size), args);
  downloadItem->file_size = file_size;
}

void DownloadContext::OnProgressInfo(int download_id,
                                     long long unsigned received,
                                     void* user_param) {
  DownloadArgs* args = static_cast<DownloadArgs*>(user_param);
  DownloadItemRefPtr downloadItem =
    args->context->downloads_[args->download_uid];

  picojson::value::object o;
  o["cmd"] = picojson::value("DownloadReplyProgress");
  o["uid"] = picojson::value(args->download_uid);
  o["receivedSize"] = picojson::value(ToString(received));
  o["totalSize"] = picojson::value(ToString(downloadItem->file_size));
  picojson::value v(o);
  args->context->api_->PostMessage(v.serialize().c_str());
}

void DownloadContext::OnFinishedInfo(int download_id, void* user_param) {
  DownloadArgs* args = static_cast<DownloadArgs*>(user_param);
  const std::string uid(args->download_uid);
  DownloadItemRefPtr downloadItem = args->context->downloads_[uid];
  char* path = 0;
  CHECK(download_get_downloaded_file_path(download_id, &path), args);
  std::string full_path = ToString(path);
  free(path);

  picojson::value::object o;
  o["cmd"] = picojson::value("DownloadReplyComplete");
  o["fullPath"] = picojson::value(full_path);
  o["uid"] = picojson::value(args->download_uid);
  picojson::value v(o);
  args->context->api_->PostMessage(v.serialize().c_str());
}

void DownloadContext::OnPausedInfo(void* user_param) {
  DownloadArgs* args = static_cast<DownloadArgs*>(user_param);

  picojson::value::object o;
  o["cmd"] = picojson::value("DownloadReplyPause");
  o["uid"] = picojson::value(args->download_uid);
  picojson::value v(o);
  args->context->api_->PostMessage(v.serialize().c_str());
}

void DownloadContext::OnCanceledInfo(void* user_param) {
  DownloadArgs* args = static_cast<DownloadArgs*>(user_param);

  picojson::value::object o;
  o["cmd"] = picojson::value("DownloadReplyCancel");
  o["uid"] = picojson::value(args->download_uid);
  picojson::value v(o);
  args->context->api_->PostMessage(v.serialize().c_str());
}

void DownloadContext::OnFailedInfo(void* user_param,
                                   const std::string& error) {
  DownloadArgs* args = static_cast<DownloadArgs*>(user_param);

  picojson::value::object o;
  o["cmd"] = picojson::value("DownloadReplyFail");
  o["uid"] = picojson::value(args->download_uid);
  o["errorCode"] = picojson::value(error);
  picojson::value v(o);
  args->context->api_->PostMessage(v.serialize().c_str());
}

void DownloadContext::HandleStart(const picojson::value& msg) {
  // Add to Downloads map
  DownloadItemRefPtr d(new DownloadItem);
  std::string uid = msg.get("uid").to_str();
  d->uid = uid;

  d->url = msg.get("url").to_str();
  d->destination = GetFullDestinationPath(msg.get("destination").to_str());
  std::string fileName = msg.get("fileName").to_str();
  d->fileName = fileName;

  std::string network_type = msg.get("networkType").to_str();
  if (network_type == "CELLULAR")
    d->networkType = DOWNLOAD_NETWORK_DATA_NETWORK;
  else if (network_type == "WIFI")
    d->networkType = DOWNLOAD_NETWORK_WIFI;
  else
    d->networkType = DOWNLOAD_NETWORK_ALL;

  DownloadArgs* args = new DownloadArgs(uid, this);
  args_.push_back(args);

  // create and start
  CHECK(download_create(&d->downloadID), args);
  CHECK(download_set_state_changed_cb(d->downloadID, OnStateChanged,
                                      static_cast<void* >(args)), args);
  CHECK(download_set_progress_cb(d->downloadID, OnProgressInfo,
                                 static_cast<void*>(args)), args);
  CHECK(download_set_url(d->downloadID, d->url.c_str()), args);
  CHECK(download_set_destination(d->downloadID, d->destination.c_str()), args);
  if (!d->fileName.empty()) {
    CHECK(download_set_file_name(d->downloadID, d->fileName.c_str()), args);
  }
  CHECK(download_set_network_type(d->downloadID, d->networkType), args);

  if (msg.get("httpHeader").is<picojson::object>()) {
    picojson::object obj = msg.get("httpHeader").get<picojson::object>();
    for (picojson::object::const_iterator it = obj.begin();
         it != obj.end(); ++it) {
      CHECK(download_add_http_header_field(d->downloadID,
            it->first.c_str(), it->second.to_str().c_str()), args);
    }
  }

  downloads_[uid] = d;  // FIXME if uid duplicate we will lose previous item

  CHECK(download_start(d->downloadID), args);
}

template <typename FnType>
bool DownloadContext::HandleGeneral(const picojson::value& msg,
                                    FnType fn,
                                    const char* fn_name) {
  int downloadID;
  DownloadArgs* args;
  if (!GetDownloadID(msg, downloadID, &args))
    return false;
  CHECK_DO(fn(downloadID), args, return false);
  return true;
}

void DownloadContext::HandleGetState(const picojson::value& msg) {
  std::string uid;
  int downloadID = -1;
  std::string retStr("DOWNLOAD_ERROR_NONE");
  download_state_e state;

  if (!GetID(msg, uid, downloadID)) {
    retStr = "DOWNLOAD_ERROR_ID_NOT_FOUND";
  } else {
    int ret = download_get_state(downloadID, &state);
    if (ret != DOWNLOAD_ERROR_NONE)
      retStr = EnumToPChar(ret);
  }

  picojson::value::object o;
  o["state"] = picojson::value(EnumToPChar(state));
  o["error"] = picojson::value(retStr);
  picojson::value v(o);
  api_->SetSyncReply(v.serialize().c_str());
}

void DownloadContext::HandleGetNetworkType(const picojson::value& msg) {
  int downloadID;
  DownloadArgs* args;
  if (!GetDownloadID(msg, downloadID, &args))
    return;

  download_network_type_e networkType;
  CHECK(download_get_network_type(downloadID, &networkType), args);

  picojson::value::object o;
  o["cmd"] = picojson::value("DownloadReplyNetworkType");
  o["uid"] = picojson::value(args->download_uid);
  o["networkType"] = picojson::value(EnumToPChar(networkType));
  picojson::value v(o);
  args->context->api_->PostMessage(v.serialize().c_str());
}

void DownloadContext::HandleGetMIMEType(const picojson::value& msg) {
  std::string uid;
  int downloadID = -1;
  std::string retStr("DOWNLOAD_ERROR_NONE");
  char* mimeType = 0;

  picojson::value::object o;
  if (!GetID(msg, uid, downloadID)) {
    retStr = "DOWNLOAD_ERROR_ID_NOT_FOUND";
  } else {
    int ret = download_get_mime_type(downloadID, &mimeType);
    if (ret != DOWNLOAD_ERROR_NONE) {
      retStr = EnumToPChar(ret);
    } else {
      o["mimeType"] = picojson::value(mimeType);
    }
  }
  o["error"] = picojson::value(retStr);
  picojson::value v(o);
  api_->SetSyncReply(v.serialize().c_str());

  if (mimeType)
    free(mimeType);
}

bool DownloadContext::GetDownloadID(const picojson::value& msg,
                                    int& downloadID, DownloadArgs** args) {
  std::string uid;
  if (!GetID(msg, uid, downloadID))
    return false;
  *args = new DownloadArgs(uid, this);
  args_.push_back(*args);
  return true;
}

bool DownloadContext::GetID(const picojson::value& msg,
                            std::string& uid,
                            int& downloadID) const {
  uid = msg.get("uid").to_str();
  if (uid == "null") {
    fprintf(stderr, "ERROR: Undefined download UID\n");
    return false;
  }
  downloadID = downloads_.find(uid)->second->downloadID;
  return true;
}

std::string DownloadContext::GetUID(int downloadID) const {
  std::string uid("null");
  for (DownloadItemMap::const_iterator it = downloads_.begin();
       it != downloads_.end(); it++) {
    if (it->second->downloadID == downloadID) {
      uid = it->second->uid;
      break;
    }
  }
  return uid;
}

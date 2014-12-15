// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "download/download_instance.h"

#include "common/picojson.h"
#include "download/download_utils.h"

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

DownloadInstance::DownloadInstance() {
}

DownloadInstance::~DownloadInstance() {
  for (DownloadArgsVector::iterator it = args_.begin();
       it != args_.end(); it++) {
    delete (*it);
  }
}

void DownloadInstance::HandleMessage(const char* msg) {
  picojson::value v;

  std::string err;
  picojson::parse(v, msg, msg + strlen(msg), &err);
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

void DownloadInstance::HandleSyncMessage(const char* msg) {
  picojson::value v;

  std::string err;
  picojson::parse(v, msg, msg + strlen(msg), &err);
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

// static
void DownloadInstance::OnStateChanged(int download_id,
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
        OnFailedInfo(args, ToString(EnumToPChar(error)));
      }
      break;
    default:
      fprintf(stderr, "Ignore state: %d\n", state);
      break;
  }
}

// static
void DownloadInstance::OnStartInfo(int download_id, void* user_param) {
  DownloadArgs* args = static_cast<DownloadArgs*>(user_param);
  const std::string uid(args->download_uid);
  DownloadItemRefPtr item = args->instance->downloads_[uid];

  long long unsigned file_size = 0;  // NOLINT
  CHECK(download_get_content_size(download_id, &file_size), args);
  item->file_size = file_size;
}

// static
void DownloadInstance::OnProgressInfo(int download_id,
                                      long long unsigned received,  // NOLINT
                                      void* user_param) {
  DownloadArgs* args = static_cast<DownloadArgs*>(user_param);
  DownloadItemRefPtr item =
    args->instance->downloads_[args->download_uid];

  picojson::value::object o;
  o["cmd"] = picojson::value("DownloadReplyProgress");
  o["uid"] = picojson::value(args->download_uid);
  o["receivedSize"] = picojson::value(ToString(received));
  o["totalSize"] = picojson::value(ToString(item->file_size));
  picojson::value v(o);
  args->instance->PostMessage(v.serialize().c_str());
}

// static
void DownloadInstance::OnFinishedInfo(int download_id, void* user_param) {
  DownloadArgs* args = static_cast<DownloadArgs*>(user_param);

  char* path = 0;
  CHECK(download_get_downloaded_file_path(download_id, &path), args);
  std::string full_path = ToString(path);
  free(path);

  picojson::value::object o;
  o["cmd"] = picojson::value("DownloadReplyComplete");
  o["fullPath"] = picojson::value(full_path);
  o["uid"] = picojson::value(args->download_uid);
  picojson::value v(o);
  args->instance->PostMessage(v.serialize().c_str());
}

// static
void DownloadInstance::OnPausedInfo(void* user_param) {
  DownloadArgs* args = static_cast<DownloadArgs*>(user_param);

  picojson::value::object o;
  o["cmd"] = picojson::value("DownloadReplyPause");
  o["uid"] = picojson::value(args->download_uid);
  picojson::value v(o);
  args->instance->PostMessage(v.serialize().c_str());
}

// static
void DownloadInstance::OnCanceledInfo(void* user_param) {
  DownloadArgs* args = static_cast<DownloadArgs*>(user_param);

  picojson::value::object o;
  o["cmd"] = picojson::value("DownloadReplyCancel");
  o["uid"] = picojson::value(args->download_uid);
  picojson::value v(o);
  args->instance->PostMessage(v.serialize().c_str());
}

// static
void DownloadInstance::OnFailedInfo(void* user_param,
                                    const std::string& error) {
  DownloadArgs* args = static_cast<DownloadArgs*>(user_param);

  picojson::value::object o;
  o["cmd"] = picojson::value("DownloadReplyFail");
  o["uid"] = picojson::value(args->download_uid);
  o["errorCode"] = picojson::value(error);
  picojson::value v(o);
  args->instance->PostMessage(v.serialize().c_str());
}

void DownloadInstance::HandleStart(const picojson::value& msg) {
  // Add to Downloads map
  DownloadItemRefPtr d(new DownloadItem);
  d->uid = msg.get("uid").to_str();
  d->url = msg.get("url").to_str();
  d->destination = GetFullDestinationPath(msg.get("destination").to_str());
  d->file_name = msg.get("fileName").to_str();

  std::string network_type = msg.get("networkType").to_str();
  if (network_type == "CELLULAR")
    d->network_type = DOWNLOAD_NETWORK_DATA_NETWORK;
  else if (network_type == "WIFI")
    d->network_type = DOWNLOAD_NETWORK_WIFI;
  else
    d->network_type = DOWNLOAD_NETWORK_ALL;

  DownloadArgs* args = new DownloadArgs(d->uid, this);
  args_.push_back(args);

  if (d->destination.empty()) {
    OnFailedInfo(args,
        ToString(EnumToPChar(DOWNLOAD_ERROR_INVALID_DESTINATION)));
    return;
  }

  // create and start
  CHECK(download_create(&d->download_id), args);
  CHECK(download_set_state_changed_cb(d->download_id, OnStateChanged,
                                      static_cast<void* >(args)), args);
  CHECK(download_set_progress_cb(d->download_id, OnProgressInfo,
                                 static_cast<void*>(args)), args);
  CHECK(download_set_url(d->download_id, d->url.c_str()), args);
  CHECK(download_set_destination(d->download_id, d->destination.c_str()), args);
  if (!d->file_name.empty()) {
    CHECK(download_set_file_name(d->download_id, d->file_name.c_str()), args);
  }
  CHECK(download_set_network_type(d->download_id, d->network_type), args);

  if (msg.get("httpHeader").is<picojson::object>()) {
    picojson::object obj = msg.get("httpHeader").get<picojson::object>();
    for (picojson::object::const_iterator it = obj.begin();
         it != obj.end(); ++it) {
      CHECK(download_add_http_header_field(d->download_id,
            it->first.c_str(), it->second.to_str().c_str()), args);
    }
  }

  downloads_[d->uid] = d;  // FIXME if uid duplicate we will lose previous item

  CHECK(download_start(d->download_id), args);
}

template <typename FnType>
bool DownloadInstance::HandleGeneral(const picojson::value& msg,
                                     FnType fn,
                                     const char* fn_name) {
  int download_id;
  DownloadArgs* args;
  if (!GetDownloadID(msg, download_id, &args))
    return false;

  CHECK_DO(fn(download_id), args, return false);
  return true;
}

void DownloadInstance::HandleGetState(const picojson::value& msg) {
  std::string uid;
  int download_id = -1;
  std::string ret_str("DOWNLOAD_ERROR_NONE");
  download_state_e state;

  if (!GetID(msg, uid, download_id)) {
    ret_str = "DOWNLOAD_ERROR_ID_NOT_FOUND";
  } else {
    int ret = download_get_state(download_id, &state);
    if (ret != DOWNLOAD_ERROR_NONE)
      ret_str = EnumToPChar(ret);
  }

  picojson::value::object o;
  o["state"] = picojson::value(EnumToPChar(state));
  o["error"] = picojson::value(ret_str);
  picojson::value v(o);
  SendSyncReply(v.serialize().c_str());
}

void DownloadInstance::HandleGetNetworkType(const picojson::value& msg) {
  int download_id;
  DownloadArgs* args;
  if (!GetDownloadID(msg, download_id, &args))
    return;

  download_network_type_e network_type;
  CHECK(download_get_network_type(download_id, &network_type), args);

  picojson::value::object o;
  o["cmd"] = picojson::value("DownloadReplyNetworkType");
  o["uid"] = picojson::value(args->download_uid);
  o["networkType"] = picojson::value(EnumToPChar(network_type));
  picojson::value v(o);
  args->instance->PostMessage(v.serialize().c_str());
}

void DownloadInstance::HandleGetMIMEType(const picojson::value& msg) {
  std::string uid;
  int download_id = -1;
  std::string ret_str("DOWNLOAD_ERROR_NONE");
  char* mime_type = 0;

  picojson::value::object o;
  if (!GetID(msg, uid, download_id)) {
    ret_str = "DOWNLOAD_ERROR_ID_NOT_FOUND";
  } else {
    int ret = download_get_mime_type(download_id, &mime_type);
    if (ret != DOWNLOAD_ERROR_NONE) {
      ret_str = EnumToPChar(ret);
    } else {
      o["mimeType"] = picojson::value(mime_type);
    }
  }

  o["error"] = picojson::value(ret_str);
  picojson::value v(o);
  SendSyncReply(v.serialize().c_str());
  free(mime_type);
}

bool DownloadInstance::GetDownloadID(const picojson::value& msg,
                                     int& download_id, DownloadArgs** args) {
  std::string uid;
  if (!GetID(msg, uid, download_id))
    return false;

  *args = new DownloadArgs(uid, this);
  args_.push_back(*args);
  return true;
}

bool DownloadInstance::GetID(const picojson::value& msg,
                             std::string& uid,
                             int& download_id) const {
  uid = msg.get("uid").to_str();
  if (uid == "null") {
    fprintf(stderr, "ERROR: Undefined download UID\n");
    return false;
  }

  download_id = downloads_.find(uid)->second->download_id;
  return true;
}

std::string DownloadInstance::GetUID(int download_id) const {
  std::string uid("null");
  for (DownloadItemMap::const_iterator it = downloads_.begin();
       it != downloads_.end(); it++) {
    if (it->second->download_id == download_id) {
      uid = it->second->uid;
      break;
    }
  }
  return uid;
}

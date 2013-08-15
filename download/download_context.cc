// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "download/download_context.h"

#include "common/picojson.h"

#define CHECK(x, args) do { \
  int retval = (x); \
  if (retval != DOWNLOAD_ERROR_NONE) { \
    fprintf(stderr, "Download error: %s returned %s at %s:%d \n", #x, \
                    ConvertErrorToString(retval), __FILE__, __LINE__); \
    OnFailedInfo(args, ToString(ConvertErrorToString(retval))); \
    return; \
  } \
} while (0)

#define CHECK_DO(x, args, y) do { \
  int retval = (x); \
  if (retval != DOWNLOAD_ERROR_NONE) { \
    fprintf(stderr, "Download error: %s returned %s at %s:%d \n", #x, \
                    ConvertErrorToString(retval), __FILE__, __LINE__); \
    OnFailedInfo(args, ToString(ConvertErrorToString(retval))); \
    y; \
  } \
} while (0)

CXWalkExtension* xwalk_extension_init(int32_t api_version) {
  return ExtensionAdapter<DownloadContext>::Initialize();
}

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

const char* DownloadContext::ConvertErrorToString(int error) {
  switch (error) {
  case DOWNLOAD_ERROR_NONE:
    return "DOWNLOAD_ERROR_NONE";
  case DOWNLOAD_ERROR_INVALID_PARAMETER:
    return "DOWNLOAD_ERROR_INVALID_PARAMETER";
  case DOWNLOAD_ERROR_OUT_OF_MEMORY:
    return "DOWNLOAD_ERROR_OUT_OF_MEMORY";
  case DOWNLOAD_ERROR_NETWORK_UNREACHABLE:
    return "DOWNLOAD_ERROR_NETWORK_UNREACHABLE";
  case DOWNLOAD_ERROR_CONNECTION_TIMED_OUT:
    return "DOWNLOAD_ERROR_CONNECTION_TIMED_OUT";
  case DOWNLOAD_ERROR_NO_SPACE:
    return "DOWNLOAD_ERROR_NO_SPACE";
  case DOWNLOAD_ERROR_FIELD_NOT_FOUND:
    return "DOWNLOAD_ERROR_FIELD_NOT_FOUND";
  case DOWNLOAD_ERROR_INVALID_STATE:
    return "DOWNLOAD_ERROR_INVALID_STATE";
  case DOWNLOAD_ERROR_CONNECTION_FAILED:
    return "DOWNLOAD_ERROR_CONNECTION_FAILED";
  case DOWNLOAD_ERROR_INVALID_URL:
    return "DOWNLOAD_ERROR_INVALID_URL";
  case DOWNLOAD_ERROR_INVALID_DESTINATION:
    return "DOWNLOAD_ERROR_INVALID_DESTINATION";
  case DOWNLOAD_ERROR_TOO_MANY_DOWNLOADS:
    return "DOWNLOAD_ERROR_TOO_MANY_DOWNLOADS";
  case DOWNLOAD_ERROR_QUEUE_FULL:
    return "DOWNLOAD_ERROR_QUEUE_FULL";
  case DOWNLOAD_ERROR_ALREADY_COMPLETED:
    return "DOWNLOAD_ERROR_ALREADY_COMPLETED";
  case DOWNLOAD_ERROR_FILE_ALREADY_EXISTS:
    return "DOWNLOAD_ERROR_FILE_ALREADY_EXISTS";
  case DOWNLOAD_ERROR_CANNOT_RESUME:
    return "DOWNLOAD_ERROR_CANNOT_RESUME";
  case DOWNLOAD_ERROR_TOO_MANY_REDIRECTS:
    return "DOWNLOAD_ERROR_TOO_MANY_REDIRECTS";
  case DOWNLOAD_ERROR_UNHANDLED_HTTP_CODE:
    return "DOWNLOAD_ERROR_UNHANDLED_HTTP_CODE";
  case DOWNLOAD_ERROR_REQUEST_TIMEOUT:
    return "DOWNLOAD_ERROR_REQUEST_TIMEOUT";
  case DOWNLOAD_ERROR_RESPONSE_TIMEOUT:
    return "DOWNLOAD_ERROR_RESPONSE_TIMEOUT";
  case DOWNLOAD_ERROR_SYSTEM_DOWN:
    return "DOWNLOAD_ERROR_SYSTEM_DOWN";
  case DOWNLOAD_ERROR_ID_NOT_FOUND:
    return "DOWNLOAD_ERROR_ID_NOT_FOUND";
  case DOWNLOAD_ERROR_NO_DATA:
    return "DOWNLOAD_ERROR_NO_DATA";
  case DOWNLOAD_ERROR_IO_ERROR:
    return "DOWNLOAD_ERROR_IO_ERROR";
  default:
    return "DOWNLOAD_UNKNOWN_ERROR";
  }
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
  // TODO(hdq): add getstate
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
      args->context->OnFailedInfo(args, ToString(ConvertErrorToString(error)));
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
  d->url  = msg.get("url").to_str();
  d->destination = GetFullDestinationPath(msg.get("destination").to_str());
  std::string filename = msg.get("filename").to_str();
  d->filename = (filename == "null") ? std::string() : filename;
  std::string uid = msg.get("uid").to_str();
  d->uid = uid;

  DownloadArgs* args = new DownloadArgs(uid, this);
  args_.push_back(args);

  // create and start
  CHECK(download_create(&d->downloadID), args);
  CHECK(download_set_state_changed_cb(d->downloadID, OnStateChanged,
                                      static_cast<void* >(args)), args);
  CHECK(download_set_progress_cb(d->downloadID, OnProgressInfo,
                                 static_cast<void*>(args)), args);
  CHECK(download_set_destination(d->downloadID, d->destination.c_str()), args);

  if (!d->filename.empty()) {
    CHECK(download_set_file_name(d->downloadID, d->filename.c_str()), args);
  }
  downloads_[uid] = d;  // FIXME if uid duplicate we will lose previous item

  CHECK(download_set_url(d->downloadID, d->url.c_str()), args);
  CHECK(download_start(d->downloadID), args);
}

template <typename FnType>
bool DownloadContext::HandleGeneral(const picojson::value& msg,
                                    FnType fn,
                                    const char* fn_name) {
  std::string uid = msg.get("uid").to_str();
  if (uid == "null") {
    fprintf(stderr, "%s - ERROR: Undefined download UID\n", fn_name);
    return false;
  }

  DownloadArgs* args = new DownloadArgs(uid, this);
  args_.push_back(args);
  int downloadID = downloads_[uid]->downloadID;
  CHECK_DO(fn(downloadID), args, return false);
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

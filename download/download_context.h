// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DOWNLOAD_DOWNLOAD_CONTEXT_H_
#define DOWNLOAD_DOWNLOAD_CONTEXT_H_

#include <tr1/memory>
#include <map>
#include <vector>
#include <string>
#include <sstream>

#include "common/extension_adapter.h"
#include "web/download.h"

namespace picojson {
class value;
}

class DownloadContext {
 public:
  explicit DownloadContext(ContextAPI* api);
  ~DownloadContext();

  // ExtensionAdapter implementation.
  static const char name[];
  static const char* GetJavaScript();
  void HandleMessage(const char* message);
  void HandleSyncMessage(const char* message) {}

 private:
  void HandleStart(const picojson::value& msg);
  template <typename FnType>
  bool HandleGeneral(const picojson::value& msg,
                     FnType fn,
                     const char* fn_name);
  struct DownloadArgs {
    std::string download_uid;
    DownloadContext* context;
    DownloadArgs(std::string uid, DownloadContext* c)
      : download_uid(uid), context(c) {}
  };
  typedef std::vector<DownloadArgs*> DownloadArgsVector;
  DownloadArgsVector args_;
  static void OnStateChanged(int download_id,
                             download_state_e state,
                             void* user_data);
  static void OnProgressInfo(int download_id,
                             long long unsigned received,
                             void* user_param);
  static void OnStartInfo(int download_id, void* user_param);
  static void OnFinishedInfo(int download_id, void* user_param);
  static void OnPausedInfo(void* user_param);
  static void OnCanceledInfo(void* user_param);
  static void OnFailedInfo(void* user_param,
                           const std::string& error);

  ContextAPI* api_;

  struct DownloadItem {
    std::string url;
    std::string destination;
    std::string filename;
    std::string uid;

    int downloadID;
    char* file_type;
    long long unsigned file_size;
    char* tmp_saved_path;
    char* content_name;

    DownloadItem() {}
    ~DownloadItem() {
      download_unset_state_changed_cb(downloadID);
      download_unset_progress_cb(downloadID);
      download_destroy(downloadID);
    }

   private:
    explicit DownloadItem(DownloadItem const&);
    void operator= (DownloadItem const&);
  };
  typedef std::tr1::shared_ptr<DownloadItem> DownloadItemRefPtr;
  typedef std::map<std::string, DownloadItemRefPtr> DownloadItemMap;
  DownloadItemMap downloads_;

  // FullDestPath = HomePath + DestPath
  // TODO(hdq): This depends on filesystem api?
  std::string GetFullDestinationPath(const std::string destination) const;

  // helpers
  template <typename T>
  static std::string ToString(T a) {
    std::ostringstream ss;
    ss << a;
    return ss.str();
  }
  std::string GetUID(int downloadID) const;
  static const char* ConvertErrorToString(int error);  // download_error_e

 private:
  explicit DownloadContext(DownloadContext const&);
  void operator=(DownloadContext const&);
};

#endif  // DOWNLOAD_DOWNLOAD_CONTEXT_H_

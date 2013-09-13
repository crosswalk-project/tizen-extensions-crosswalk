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
#include "common/utils.h"
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
  void HandleSyncMessage(const char* message);

 private:
  void HandleStart(const picojson::value& msg);
  template <typename FnType>
  bool HandleGeneral(const picojson::value& msg,
                     FnType fn,
                     const char* fn_name);
  void HandleGetState(const picojson::value& msg);
  void HandleGetNetworkType(const picojson::value& msg);
  void HandleGetMIMEType(const picojson::value& msg);
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
    std::string uid;
    std::string url;
    std::string destination;
    std::string fileName;
    download_network_type_e networkType;
    std::string httpHeader;

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
    DISALLOW_COPY_AND_ASSIGN(DownloadItem);
  };
  typedef std::tr1::shared_ptr<DownloadItem> DownloadItemRefPtr;
  typedef std::map<std::string, DownloadItemRefPtr> DownloadItemMap;
  DownloadItemMap downloads_;

  // FullDestPath = HomePath + DestPath
  // TODO(hdq): This depends on filesystem api?
  const std::string GetFullDestinationPath(const std::string destination) const;
  const std::string GetRealLocation(const std::string& destination) const;

  bool GetDownloadID(const picojson::value& msg,
                     int& downloadID, DownloadArgs** args);
  bool GetID(const picojson::value& msg,
             std::string& uid, int& downloadID) const;
  std::string GetUID(int downloadID) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadContext);
};

#endif  // DOWNLOAD_DOWNLOAD_CONTEXT_H_

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DOWNLOAD_DOWNLOAD_INSTANCE_H_
#define DOWNLOAD_DOWNLOAD_INSTANCE_H_

#include <tr1/memory>
#include <map>
#include <vector>
#include <string>
#include <sstream>

#include "common/extension.h"
#include "common/utils.h"
#include "common/virtual_fs.h"
#include "web/download.h"

namespace picojson {

class value;

}  // namespace picojson

class DownloadInstance : public common::Instance {
 public:
  DownloadInstance();
  ~DownloadInstance();

 private:
  VirtualFS vfs_;
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* msg);

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
    DownloadInstance* instance;
    DownloadArgs(std::string uid, DownloadInstance* i)
      : download_uid(uid), instance(i) {}
  };
  typedef std::vector<DownloadArgs*> DownloadArgsVector;
  DownloadArgsVector args_;
  static void OnStateChanged(int download_id,
                             download_state_e state,
                             void* user_data);
  static void OnProgressInfo(int download_id,
                             long long unsigned received,  // NOLINT
                             void* user_param);
  static void OnStartInfo(int download_id, void* user_param);
  static void OnFinishedInfo(int download_id, void* user_param);
  static void OnPausedInfo(void* user_param);
  static void OnCanceledInfo(void* user_param);
  static void OnFailedInfo(void* user_param,
                           const std::string& error);

  struct DownloadItem {
    std::string uid;
    std::string url;
    std::string destination;
    std::string file_name;
    download_network_type_e network_type;
    std::string http_header;

    int download_id;
    char* file_type;
    long long unsigned file_size;  // NOLINT
    char* tmp_saved_path;
    char* content_name;

    DownloadItem() {}
    ~DownloadItem() {
      download_unset_state_changed_cb(download_id);
      download_unset_progress_cb(download_id);
      download_destroy(download_id);
    }

   private:
    DISALLOW_COPY_AND_ASSIGN(DownloadItem);
  };
  typedef std::tr1::shared_ptr<DownloadItem> DownloadItemRefPtr;
  typedef std::map<std::string, DownloadItemRefPtr> DownloadItemMap;
  DownloadItemMap downloads_;

  const std::string GetFullDestinationPath(const std::string destination) const;

  bool GetDownloadID(const picojson::value& msg,
                     int& download_id, DownloadArgs** args);
  bool GetID(const picojson::value& msg,
             std::string& uid, int& download_id) const;
  std::string GetUID(int download_id) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadInstance);
};

#endif  // DOWNLOAD_DOWNLOAD_INSTANCE_H_

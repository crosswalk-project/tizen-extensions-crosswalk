// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FILESYSTEM_FILESYSTEM_INSTANCE_H_
#define FILESYSTEM_FILESYSTEM_INSTANCE_H_

#include <app_storage.h>

#include <set>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

#include "common/extension.h"
#include "common/picojson.h"
#include "tizen/tizen.h"

class FilesystemInstance : public common::Instance {
 public:
  FilesystemInstance();
  ~FilesystemInstance();

  // common::Instance implementation
  void HandleMessage(const char* message);
  void HandleSyncMessage(const char* message);

 private:
  class Storage {
   public:
    /* Mapped to storage_type_e */
    enum StorageType {
      STORAGE_TYPE_INTERNAL,
      STORAGE_TYPE_EXTERNAL,
    };

    /* Mapped to storage_state_e */
    enum StorageState {
      STORAGE_STATE_UNMOUNTABLE = -2,
      STORAGE_STATE_REMOVED = -1,
      STORAGE_STATE_MOUNTED = 0,
      STORAGE_STATE_MOUNTED_READONLY = 1,
    };

    Storage(int id, int type, int state, const std::string& fullpath);

    picojson::object toJSON(const std::string& label) const;

    std::string type() const;
    std::string state() const;
    int GetId() const { return id_; }
    const std::string& GetFullPath() const { return fullpath_; }
    void SetState(int state) { state_ = state; }

   private:
    int id_;
    int type_;
    int state_;
    std::string fullpath_;
  };

  /* Asynchronous messages */
  void HandleFileSystemManagerResolve(const picojson::value& msg);
  void HandleFileSystemManagerGetStorage(const picojson::value& msg);
  void HandleFileSystemManagerListStorages(const picojson::value& msg);
  void HandleFileOpenStream(const picojson::value& msg);
  void HandleFileDeleteDirectory(const picojson::value& msg);
  void HandleFileDeleteFile(const picojson::value& msg);
  void HandleFileListFiles(const picojson::value& msg);
  void HandleFileCopyTo(const picojson::value& msg);
  void HandleFileMoveTo(const picojson::value& msg);

  /* Asynchronous message helpers */
  void PostAsyncErrorReply(const picojson::value&, WebApiAPIErrors);
  void PostAsyncSuccessReply(const picojson::value&, picojson::value::object&);
  void PostAsyncSuccessReply(const picojson::value&, picojson::value&);
  void PostAsyncSuccessReply(const picojson::value&, WebApiAPIErrors);
  void PostAsyncSuccessReply(const picojson::value&);

  /* Sync messages */
  void HandleFileSystemManagerGetMaxPathLength(const picojson::value& msg,
                                               std::string& reply);
  void HandleFileStreamClose(const picojson::value& msg, std::string& reply);
  void HandleFileStreamRead(const picojson::value& msg, std::string& reply);
  void HandleFileStreamWrite(const picojson::value& msg, std::string& reply);
  void HandleFileCreateDirectory(const picojson::value& msg,
                                 std::string& reply);
  void HandleFileCreateFile(const picojson::value& msg, std::string& reply);
  void HandleFileGetURI(const picojson::value& msg, std::string& reply);
  void HandleFileResolve(const picojson::value& msg, std::string& reply);
  void HandleFileStat(const picojson::value& msg, std::string& reply);
  void HandleFileStreamStat(const picojson::value& msg, std::string& reply);
  void HandleFileStreamSetPosition(const picojson::value& msg,
                                   std::string& reply);

  /* Sync message helpers */
  bool IsKnownFileStream(const picojson::value& msg);
  std::fstream* GetFileStream(unsigned int key);
  std::fstream* GetFileStream(unsigned int key, std::ios_base::openmode mode);
  bool CopyAndRenameSanityChecks(const picojson::value& msg,
      const std::string& from, const std::string& to, bool overwrite);
  void SetSyncError(std::string& output, WebApiAPIErrors error_type);
  void SetSyncSuccess(std::string& reply);
  void SetSyncSuccess(std::string& reply, std::string& output);
  void SetSyncSuccess(std::string& reply, picojson::value& output);

  std::string GetRealPath(const std::string& fullPath);
  void AddInternalStorage(const std::string& label, const std::string& path);
  void AddStorage(int storage, storage_type_e type, storage_state_e state,
      const std::string& path);
  void NotifyStorageStateChanged(int id, storage_state_e state);
  static bool OnStorageDeviceSupported(int id, storage_type_e type,
      storage_state_e state, const char *path, void *user_data);
  static void OnStorageStateChanged(int id, storage_state_e state,
      void *user_data);

  typedef std::pair<std::ios_base::openmode, std::fstream*> FStream;
  typedef std::map<unsigned int, FStream> FStreamMap;
  FStreamMap fstream_map_;
  typedef std::map<std::string, Storage> Storages;
  typedef std::pair<std::string, Storage> SorageLabelPair;
  Storages storages_;
  std::vector<int> watched_storages_;
};

#endif  // FILESYSTEM_FILESYSTEM_INSTANCE_H_

// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FILESYSTEM_FILESYSTEM_INSTANCE_H_
#define FILESYSTEM_FILESYSTEM_INSTANCE_H_

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>

#include "common/extension.h"
#include "common/picojson.h"
#include "common/virtual_fs.h"
#include "tizen/tizen.h"

class FilesystemInstance : public common::Instance {
 public:
  FilesystemInstance();
  ~FilesystemInstance();

  // common::Instance implementation
  void Initialize();
  void HandleMessage(const char* message);
  void HandleSyncMessage(const char* message);

 private:
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
  std::string GetFileEncoding(unsigned int key) const;
  void ReadText(std::fstream* file, size_t num_chars, const char* encoding,
      std::string& reply);
  std::string ResolveImplicitDestination(const std::string& from,
      const std::string& to);
  bool CopyAndRenameSanityChecks(const picojson::value& msg,
      const std::string& from, const std::string& to, bool overwrite);
  void SetSyncError(std::string& output, WebApiAPIErrors error_type);
  void SetSyncSuccess(std::string& reply);
  void SetSyncSuccess(std::string& reply, std::string& output);
  void SetSyncSuccess(std::string& reply, picojson::value& output);

  void NotifyStorageStateChanged(const std::string& label, Storage storage);
  static void OnStorageStateChanged(const std::string& label, Storage storage,
      void* user_data);

  typedef std::tuple<std::ios_base::openmode, std::fstream*,
      std::string> FStream;
  typedef std::map<unsigned int, FStream> FStreamMap;
  FStreamMap fstream_map_;
  VirtualFS vfs_;
};

#endif  // FILESYSTEM_FILESYSTEM_INSTANCE_H_

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FILESYSTEM_FILESYSTEM_CONTEXT_H_
#define FILESYSTEM_FILESYSTEM_CONTEXT_H_

#include <set>
#include <string>

#include "common/extension_adapter.h"
#include "common/picojson.h"
#include "tizen/tizen.h"

class FilesystemContext {
 public:
  explicit FilesystemContext(ContextAPI* api);
  ~FilesystemContext();

  /* ExtensionAdapter implementation */
  static const char name[];
  static const char* GetJavaScript();
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
  void HandleFileStreamReadBytes(const picojson::value& msg,
        std::string& reply);
  void HandleFileStreamReadBase64(const picojson::value& msg,
        std::string& reply);
  void HandleFileStreamWrite(const picojson::value& msg, std::string& reply);
  void HandleFileStreamWriteBytes(const picojson::value& msg,
        std::string& reply);
  void HandleFileStreamWriteBase64(const picojson::value& msg,
        std::string& reply);
  void HandleFileCreateDirectory(const picojson::value& msg,
        std::string& reply);
  void HandleFileCreateFile(const picojson::value& msg, std::string& reply);
  void HandleFileResolve(const picojson::value& msg, std::string& reply);
  void HandleFileStat(const picojson::value& msg, std::string& reply);
  void HandleFileGetFullPath(const picojson::value& msg, std::string& reply);

  /* Sync message helpers */
  bool IsKnownFileDescriptor(int fd);
  bool CopyAndRenameSanityChecks(const picojson::value& msg,
        const std::string& from, const std::string& to, bool overwrite);
  void SetSyncError(std::string& output, WebApiAPIErrors error_type);
  void SetSyncSuccess(std::string& reply);
  void SetSyncSuccess(std::string& reply, std::string& output);
  void SetSyncSuccess(std::string& reply, picojson::value& output);

  ContextAPI* api_;
  std::set<int> known_file_descriptors_;
};

#endif  // FILESYSTEM_FILESYSTEM_CONTEXT_H_

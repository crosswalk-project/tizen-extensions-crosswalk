// Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DATASYNC_DATASYNC_INSTANCE_H_
#define DATASYNC_DATASYNC_INSTANCE_H_

#include <string>

#include "common/extension.h"
#include "common/picojson.h"

#include "datasync/datasync_extension.h"
#include "datasync/datasync_manager.h"

namespace datasync {

class DatasyncInstance : public common::Instance {
 public:
  explicit DatasyncInstance(DatasyncExtension& extension);
  virtual ~DatasyncInstance();

  void ReplyAsyncOnCompleted(int key, const std::string& profile_id);
  void ReplyAsyncOnStopped(int key, const std::string& profile_id);
  void ReplyAsyncOnFailed(int key,
      const std::string& profile_id,
      int error_code,
      const std::string& name,
      const std::string& message);
  void ReplyAsyncOnProgress(int key,
      const std::string& profile_id,
      bool is_from_server,
      int synced_per_db,
      int total_per_db,
      SyncServiceInfo::SyncServiceType type);

 private:
  void HandleMessage(const char* msg);
  void HandleSyncMessage(const char* msg);

  void ReplySyncAnswer(const picojson::value& value);
  void ReplySyncUndefinedAnswer();
  void ReplySyncException(unsigned code, const std::string& name,
      const std::string& message);
  void MakeExceptionAndReply(const Error& e);

  void ReplyAsyncAnswer(int key, const std::string& name,
      const picojson::value& returnValue);

  // Synchronous message handlers

  // with result
  void HandleGetMaxProfilesNum(const picojson::value& arg);
  void HandleGetProfilesNum(const picojson::value& arg);
  void HandleGet(const picojson::value& arg);
  void HandleGetAll(const picojson::value& arg);
  void HandleGetLastSyncStatistics(const picojson::value& arg);

  // undefined result
  void HandleAdd(const picojson::value& arg);
  void HandleUpdate(const picojson::value& arg);
  void HandleRemove(const picojson::value& arg);
  void HandleStartSync(const picojson::value& arg, int callback_id);
  void HandleStopSync(const picojson::value& arg);

  DatasyncExtension& extension_;
};

}  // namespace datasync

#endif  // DATASYNC_DATASYNC_INSTANCE_H_

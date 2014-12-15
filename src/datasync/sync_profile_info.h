// Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DATASYNC_SYNC_PROFILE_INFO_H_
#define DATASYNC_SYNC_PROFILE_INFO_H_

#include <string>
#include <vector>

#include "datasync/sync_info.h"
#include "datasync/sync_service_info.h"

namespace datasync {

/**
 * Representation of SyncProfileInfo JS type
 */
class SyncProfileInfo {
 public:
  SyncProfileInfo();
  SyncProfileInfo(const std::string& profileId, const std::string& profile_name,
                  SyncInfoPtr sync_info, SyncServiceInfoListPtr service_info);

  std::string profile_id() const;
  void set_profile_id(const std::string &value);

  std::string profile_name() const;
  void set_profile_name(const std::string &value);

  SyncInfoPtr sync_info() const;
  void set_sync_info(SyncInfoPtr value);

  SyncServiceInfoListPtr service_info() const;
  void set_service_info(SyncServiceInfoListPtr value);

 protected:
  std::string profile_id_;
  std::string profile_name_;
  SyncInfoPtr sync_info_;
  SyncServiceInfoListPtr service_info_;
};

typedef std::shared_ptr<SyncProfileInfo> SyncProfileInfoPtr;
typedef std::vector<SyncProfileInfoPtr> SyncProfileInfoList;
typedef std::shared_ptr<SyncProfileInfoList> SyncProfileInfoListPtr;

}  // namespace datasync

#endif  // DATASYNC_SYNC_PROFILE_INFO_H_

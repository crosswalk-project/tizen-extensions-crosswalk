// Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "datasync/sync_profile_info.h"

namespace datasync {

SyncProfileInfo::SyncProfileInfo()
    : sync_info_(new SyncInfo()),
      service_info_(new SyncServiceInfoList()) {
}

SyncProfileInfo::SyncProfileInfo(const std::string& profileId,
    const std::string& profile_name,
    SyncInfoPtr sync_info,
    SyncServiceInfoListPtr service_info)
    : profile_id_(profileId),
      profile_name_(profile_name),
      sync_info_(sync_info),
      service_info_(service_info) {
}

std::string SyncProfileInfo::profile_id() const { return profile_id_; }

void SyncProfileInfo::set_profile_id(const std::string &profileId) {
  profile_id_ = profileId;
}

std::string SyncProfileInfo::profile_name() const { return profile_name_; }

void SyncProfileInfo::set_profile_name(const std::string &profileName) {
  profile_name_ = profileName;
}

SyncInfoPtr SyncProfileInfo::sync_info() const { return sync_info_; }

void SyncProfileInfo::set_sync_info(SyncInfoPtr syncInfo) {
  sync_info_ = syncInfo;
}

SyncServiceInfoListPtr SyncProfileInfo::service_info() const {
  return service_info_;
}

void SyncProfileInfo::set_service_info(SyncServiceInfoListPtr serviceInfo) {
  service_info_ = serviceInfo;
}

}  // namespace datasync

// Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "datasync/sync_info.h"

namespace datasync {

std::string SyncInfo::SyncModeToString(SyncMode mode) {
  switch (mode) {
    case MANUAL_MODE:
      return "MANUAL";
    case PERIODIC_MODE:
      return "PERIODIC";
    case PUSH_MODE:
      return "PUSH";
    default:
      return "";
  }
}

std::string SyncInfo::SyncTypeToString(SyncType type) {
  switch (type) {
    case TWO_WAY_TYPE:
      return "TWO_WAY";
    case SLOW_TYPE:
      return "SLOW";
    case ONE_WAY_FROM_CLIENT_TYPE:
      return "ONE_WAY_FROM_CLIENT";
    case REFRESH_FROM_CLIENT_TYPE:
      return "REFRESH_FROM_CLIENT";
    case ONE_WAY_FROM_SERVER_TYPE:
      return "ONE_WAY_FROM_SERVER";
    case REFRESH_FROM_SERVER_TYPE:
      return "REFRESH_FROM_SERVER";
    default:
      return "";
  }
}

std::string SyncInfo::SyncIntervalToString(SyncInterval interval) {
  switch (interval) {
    case INTERVAL_5_MINUTES:
      return "5_MINUTES";
    case INTERVAL_15_MINUTES:
      return "15_MINUTES";
    case INTERVAL_1_HOUR:
      return "1_HOUR";
    case INTERVAL_4_HOURS:
      return "4_HOURS";
    case INTERVAL_12_HOURS:
      return "12_HOURS";
    case INTERVAL_1_DAY:
      return "1_DAY";
    case INTERVAL_1_WEEK:
      return "1_WEEK";
    case INTERVAL_1_MONTH:
      return "1_MONTH";
    default:
      return "";
  }
}

SyncInfo::SyncMode SyncInfo::ConvertToSyncMode(const std::string& str) {
  if (str == "MANUAL") {
    return MANUAL_MODE;
  } else if (str == "PERIODIC") {
    return PERIODIC_MODE;
  } else if (str == "PUSH") {
    return PUSH_MODE;
  } else {
    return UNDEFINED_MODE;
  }
}

SyncInfo::SyncType SyncInfo::ConvertToSyncType(const std::string& str) {
  if (str == "TWO_WAY") {
    return TWO_WAY_TYPE;
  } else if (str == "SLOW") {
    return SLOW_TYPE;
  } else if (str == "ONE_WAY_FROM_CLIENT") {
    return ONE_WAY_FROM_CLIENT_TYPE;
  } else if (str == "REFRESH_FROM_CLIENT") {
    return REFRESH_FROM_CLIENT_TYPE;
  } else if (str == "ONE_WAY_FROM_SERVER") {
    return ONE_WAY_FROM_SERVER_TYPE;
  } else if (str == "REFRESH_FROM_SERVER") {
    return REFRESH_FROM_SERVER_TYPE;
  } else {
    return UNDEFINED_TYPE;
  }
}

SyncInfo::SyncInterval SyncInfo::ConvertToSyncInterval(const std::string& str) {
  if (str == "5_MINUTES") {
    return INTERVAL_5_MINUTES;
  } else if (str == "15_MINUTES") {
    return INTERVAL_15_MINUTES;
  } else if (str == "1_HOUR") {
    return INTERVAL_1_HOUR;
  } else if (str == "4_HOURS") {
    return INTERVAL_4_HOURS;
  } else if (str == "12_HOURS") {
    return INTERVAL_12_HOURS;
  } else if (str == "1_DAY") {
    return INTERVAL_1_DAY;
  } else if (str == "1_WEEK") {
    return INTERVAL_1_WEEK;
  } else if (str == "1_MONTH") {
    return INTERVAL_1_MONTH;
  } else {
    return INTERVAL_UNDEFINED;
  }
}

SyncInfo::SyncInfo()
    : sync_mode_(UNDEFINED_MODE),
      sync_type_(UNDEFINED_TYPE),
      sync_interval_(INTERVAL_UNDEFINED) {
}

SyncInfo::SyncInfo(std::string url, std::string id, std::string password,
    SyncMode sync_mode, SyncType sync_type,
    SyncInterval sync_interval)
    : url_(url),
      id_(id),
      password_(password),
      sync_mode_(sync_mode),
      sync_type_(sync_type),
      sync_interval_(sync_interval) {
}

SyncInfo::~SyncInfo() {}

std::string SyncInfo::url() const { return url_; }

void SyncInfo::set_url(const std::string& url) { url_ = url; }

std::string SyncInfo::id() const { return id_; }

void SyncInfo::set_id(const std::string& id) { id_ = id; }

std::string SyncInfo::password() const { return password_; }

void SyncInfo::set_password(const std::string& password) {
  password_ = password;
}

SyncInfo::SyncMode SyncInfo::sync_mode() const { return sync_mode_; }

void SyncInfo::set_sync_mode(SyncInfo::SyncMode syncMode) {
  sync_mode_ = syncMode;
}

SyncInfo::SyncType SyncInfo::sync_type() const { return sync_type_; }

void SyncInfo::set_sync_type(SyncInfo::SyncType syncType) {
  sync_type_ = syncType;
}

SyncInfo::SyncInterval SyncInfo::sync_interval() const {
  return sync_interval_;
}

void SyncInfo::set_sync_interval(SyncInfo::SyncInterval syncInterval) {
  sync_interval_ = syncInterval;
}

}  // namespace datasync

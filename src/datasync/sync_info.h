// Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DATASYNC_SYNC_INFO_H_
#define DATASYNC_SYNC_INFO_H_

#include <memory>
#include <string>

namespace datasync {

/**
 * Representation of SyncInfo JS type
 */
class SyncInfo {
 public:
  typedef enum {
    MANUAL_MODE,
    PERIODIC_MODE,
    PUSH_MODE,
    UNDEFINED_MODE
  } SyncMode;

  typedef enum {
    TWO_WAY_TYPE,
    SLOW_TYPE,
    ONE_WAY_FROM_CLIENT_TYPE,
    REFRESH_FROM_CLIENT_TYPE,
    ONE_WAY_FROM_SERVER_TYPE,
    REFRESH_FROM_SERVER_TYPE,
    UNDEFINED_TYPE
  } SyncType;

  typedef enum {
    INTERVAL_5_MINUTES,
    INTERVAL_15_MINUTES,
    INTERVAL_1_HOUR,
    INTERVAL_4_HOURS,
    INTERVAL_12_HOURS,
    INTERVAL_1_DAY,
    INTERVAL_1_WEEK,
    INTERVAL_1_MONTH,
    INTERVAL_UNDEFINED
  } SyncInterval;

  static std::string SyncModeToString(SyncMode mode);
  static std::string SyncTypeToString(SyncType type);
  static std::string SyncIntervalToString(SyncInterval interval);

  static SyncMode ConvertToSyncMode(const std::string& str);
  static SyncType ConvertToSyncType(const std::string& str);
  static SyncInterval ConvertToSyncInterval(const std::string& str);

  SyncInfo();
  SyncInfo(std::string url, std::string id, std::string password,
           SyncMode sync_mode, SyncType sync_type, SyncInterval sync_interval);
  virtual ~SyncInfo();

  std::string url() const;
  void set_url(const std::string& value);

  std::string id() const;
  void set_id(const std::string& value);

  std::string password() const;
  void set_password(const std::string& value);

  SyncMode sync_mode() const;
  void set_sync_mode(SyncMode value);

  SyncType sync_type() const;
  void set_sync_type(SyncType value);

  SyncInterval sync_interval() const;
  void set_sync_interval(SyncInterval value);

 protected:
  std::string url_;
  std::string id_;
  std::string password_;
  SyncMode sync_mode_;
  SyncType sync_type_;
  SyncInterval sync_interval_;
};

typedef std::shared_ptr<SyncInfo> SyncInfoPtr;

}  // namespace datasync

#endif  // DATASYNC_SYNC_INFO_H_

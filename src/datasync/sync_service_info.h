// Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DATASYNC_SYNC_SERVICE_INFO_H_
#define DATASYNC_SYNC_SERVICE_INFO_H_

#include <memory>
#include <string>
#include <vector>

namespace datasync {

/**
 * Representation of SyncServiceInfo JS type
 */
class SyncServiceInfo {
 public:
  typedef enum {
    CONTACT_SERVICE_TYPE,
    EVENT_SERVICE_TYPE,
    UNDEFINED_SERVICE_TYPE,
    INVALID
  } SyncServiceType;

  static SyncServiceType ConvertToSyncServiceType(const std::string &type);
  static std::string SyncServiceTypeToString(SyncServiceType type);

  SyncServiceInfo();
  SyncServiceInfo(bool enable, SyncServiceType sync_service_type,
                  std::string server_database_uri, std::string id,
                  std::string password);

  bool enable() const;
  void set_enable(bool value);

  SyncServiceType sync_service_type() const;
  void set_sync_service_type(SyncServiceType value);

  std::string server_database_uri() const;
  void set_server_database_uri(const std::string &value);

  std::string id() const;
  void set_id(const std::string &value);

  std::string password() const;
  void set_password(const std::string &value);

 protected:
  bool enable_;
  SyncServiceType sync_service_type_;
  std::string server_database_uri_;
  std::string id_;
  std::string password_;
};

typedef std::shared_ptr<SyncServiceInfo> SyncServiceInfoPtr;
typedef std::vector<SyncServiceInfoPtr> SyncServiceInfoList;
typedef std::shared_ptr<SyncServiceInfoList> SyncServiceInfoListPtr;

}  // namespace datasync

#endif  // DATASYNC_SYNC_SERVICE_INFO_H_

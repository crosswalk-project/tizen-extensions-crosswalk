// Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "datasync/sync_service_info.h"

namespace datasync {

SyncServiceInfo::SyncServiceType SyncServiceInfo::ConvertToSyncServiceType(
    const std::string &type) {
  if (type == "CONTACT") {
    return CONTACT_SERVICE_TYPE;
  } else if (type == "EVENT") {
    return EVENT_SERVICE_TYPE;
  } else {
    return INVALID;
  }
}

std::string SyncServiceInfo::SyncServiceTypeToString(SyncServiceType type) {
  switch (type) {
    case CONTACT_SERVICE_TYPE:
      return "CONTACT";
    case EVENT_SERVICE_TYPE:
      return "EVENT";
    default:
      return "";
  }
}

SyncServiceInfo::SyncServiceInfo()
    : enable_(true),
      sync_service_type_(UNDEFINED_SERVICE_TYPE) {
}

SyncServiceInfo::SyncServiceInfo(bool enable,
    SyncServiceType sync_service_type,
    std::string server_database_uri,
    std::string id,
    std::string password)
    : enable_(enable),
      sync_service_type_(sync_service_type),
      server_database_uri_(server_database_uri),
      id_(id),
      password_(password) {
}

bool SyncServiceInfo::enable() const { return enable_; }

void SyncServiceInfo::set_enable(bool enable) { enable_ = enable; }

SyncServiceInfo::SyncServiceType SyncServiceInfo::sync_service_type() const {
  return sync_service_type_;
}

void SyncServiceInfo::set_sync_service_type(
    SyncServiceInfo::SyncServiceType syncServiceType) {
  sync_service_type_ = syncServiceType;
}

std::string SyncServiceInfo::server_database_uri() const {
  return server_database_uri_;
}

void SyncServiceInfo::set_server_database_uri(const std::string &uri) {
  server_database_uri_ = uri;
}

std::string SyncServiceInfo::id() const { return id_; }

void SyncServiceInfo::set_id(const std::string &id) { id_ = id; }

std::string SyncServiceInfo::password() const { return password_; }

void SyncServiceInfo::set_password(const std::string &password) {
  password_ = password;
}

}  // namespace datasync

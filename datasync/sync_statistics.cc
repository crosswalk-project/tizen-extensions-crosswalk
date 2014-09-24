// Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "datasync/sync_statistics.h"

namespace {

const int kUndefinedTime = -1;

}

namespace datasync {

SyncStatistics::SyncStatus SyncStatistics::ConvertToSyncStatus(
    const std::string& status) {
  if (status == "SUCCESS") {
    return SUCCESS_STATUS;
  } else if (status == "FAIL") {
    return FAIL_STATUS;
  } else if (status == "STOP") {
    return STOP_STATUS;
  } else if (status == "NONE") {
    return NONE_STATUS;
  } else {
    return INVALID;
  }
}

std::string SyncStatistics::SyncStatusToString(SyncStatus status) {
  switch (status) {
    case SUCCESS_STATUS:
      return "SUCCESS";
    case FAIL_STATUS:
      return "FAIL";
    case STOP_STATUS:
      return "STOP";
    case NONE_STATUS:
      return "NONE";
    default:
      return "";
  }
}

SyncStatistics::SyncStatistics()
    : sync_status_(NONE_STATUS),
      service_type_(SyncServiceInfo::UNDEFINED_SERVICE_TYPE),
      last_sync_time_(kUndefinedTime),
      server_to_client_total_(0),
      server_to_client_added_(0),
      server_to_client_updated_(0),
      server_to_client_removed_(0),
      client_to_server_total_(0),
      client_to_server_added_(0),
      client_to_server_updated_(0),
      client_to_server_removed_(0) {
}

SyncStatistics::SyncStatistics(
    SyncStatus sync_status, SyncServiceInfo::SyncServiceType service_type,
    unsigned last_sync_time, unsigned server_to_client_total,
    unsigned server_to_client_added, unsigned server_to_client_updated,
    unsigned server_to_client_removed, unsigned client_to_server_total,
    unsigned client_to_server_added, unsigned client_to_server_updated,
    unsigned client_to_server_removed)
    : sync_status_(sync_status),
      service_type_(service_type),
      last_sync_time_(last_sync_time),
      server_to_client_total_(server_to_client_total),
      server_to_client_added_(server_to_client_added),
      server_to_client_updated_(server_to_client_updated),
      server_to_client_removed_(server_to_client_removed),
      client_to_server_total_(client_to_server_total),
      client_to_server_added_(client_to_server_added),
      client_to_server_updated_(client_to_server_updated),
      client_to_server_removed_(client_to_server_removed) {
}

SyncStatistics::SyncStatus SyncStatistics::sync_status() const {
  return sync_status_;
}

void SyncStatistics::set_sync_status(SyncStatistics::SyncStatus sync_status) {
  sync_status_ = sync_status;
}

SyncServiceInfo::SyncServiceType SyncStatistics::service_type() const {
  return service_type_;
}

void SyncStatistics::set_service_type(
    SyncServiceInfo::SyncServiceType service_type) {
  service_type_ = service_type;
}

unsigned SyncStatistics::last_sync_time() const { return last_sync_time_; }

void SyncStatistics::set_last_sync_time(unsigned value) {
  last_sync_time_ = value;
}

unsigned SyncStatistics::server_to_client_total() const {
  return server_to_client_total_;
}

void SyncStatistics::set_server_to_client_total(unsigned value) {
  server_to_client_total_ = value;
}

unsigned SyncStatistics::server_to_client_added() const {
  return server_to_client_added_;
}

void SyncStatistics::set_server_to_client_added(unsigned value) {
  server_to_client_added_ = value;
}

unsigned SyncStatistics::server_to_client_updated() const {
  return server_to_client_updated_;
}

void SyncStatistics::set_server_to_client_updated(unsigned value) {
  server_to_client_updated_ = value;
}

unsigned SyncStatistics::server_to_client_removed() const {
  return server_to_client_removed_;
}

void SyncStatistics::set_server_to_client_removed(unsigned value) {
  server_to_client_removed_ = value;
}

unsigned SyncStatistics::client_to_server_total() const {
  return client_to_server_total_;
}

void SyncStatistics::set_client_to_server_total(unsigned value) {
  client_to_server_total_ = value;
}

unsigned SyncStatistics::client_to_server_added() const {
  return client_to_server_added_;
}

void SyncStatistics::set_client_to_server_added(unsigned value) {
  client_to_server_added_ = value;
}

unsigned SyncStatistics::client_to_server_updated() const {
  return client_to_server_updated_;
}

void SyncStatistics::set_client_to_server_updated(unsigned value) {
  client_to_server_updated_ = value;
}

unsigned SyncStatistics::client_to_server_removed() const {
  return client_to_server_removed_;
}

void SyncStatistics::set_client_to_server_removed(unsigned value) {
  client_to_server_removed_ = value;
}

}  // namespace datasync

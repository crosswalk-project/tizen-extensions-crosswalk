// Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DATASYNC_SYNC_STATISTICS_H_
#define DATASYNC_SYNC_STATISTICS_H_

#include <string>
#include <vector>

#include "datasync/sync_service_info.h"

namespace datasync {

/**
 *Representation of SyncStatistics JS type
 */
class SyncStatistics {
 public:
  typedef enum {
    SUCCESS_STATUS,
    FAIL_STATUS,
    STOP_STATUS,
    NONE_STATUS,
    INVALID
  } SyncStatus;

  static SyncStatus ConvertToSyncStatus(const std::string& status);
  static std::string SyncStatusToString(SyncStatus status);

  SyncStatistics();
  SyncStatistics(SyncStatus sync_status,
       SyncServiceInfo::SyncServiceType service_type,
       unsigned last_sync_time,
       unsigned server_to_client_total,
       unsigned server_to_client_added,
       unsigned server_to_client_updated,
       unsigned server_to_client_removed,
       unsigned client_to_server_total,
       unsigned client_to_server_added,
       unsigned client_to_server_updated,
       unsigned client_to_server_removed);

  SyncStatus sync_status() const;
  void set_sync_status(SyncStatus sync_status);

  SyncServiceInfo::SyncServiceType service_type() const;
  void set_service_type(SyncServiceInfo::SyncServiceType service_type);

  unsigned last_sync_time() const;
  void set_last_sync_time(unsigned value);

  unsigned server_to_client_total() const;
  void set_server_to_client_total(unsigned value);

  unsigned server_to_client_added() const;
  void set_server_to_client_added(unsigned value);

  unsigned server_to_client_updated() const;
  void set_server_to_client_updated(unsigned value);

  unsigned server_to_client_removed() const;
  void set_server_to_client_removed(unsigned value);

  unsigned client_to_server_total() const;
  void set_client_to_server_total(unsigned value);

  unsigned client_to_server_added() const;
  void set_client_to_server_added(unsigned value);

  unsigned client_to_server_updated() const;
  void set_client_to_server_updated(unsigned value);

  unsigned client_to_server_removed() const;
  void set_client_to_server_removed(unsigned value);

 private:
  SyncStatus sync_status_;
  SyncServiceInfo::SyncServiceType service_type_;
  unsigned last_sync_time_;
  unsigned server_to_client_total_;
  unsigned server_to_client_added_;
  unsigned server_to_client_updated_;
  unsigned server_to_client_removed_;
  unsigned client_to_server_total_;
  unsigned client_to_server_added_;
  unsigned client_to_server_updated_;
  unsigned client_to_server_removed_;
};

typedef std::shared_ptr<SyncStatistics> SyncStatisticsPtr;
typedef std::vector<SyncStatisticsPtr> SyncStatisticsList;
typedef std::shared_ptr<SyncStatisticsList> SyncStatisticsListPtr;

}  // namespace datasync

#endif  // DATASYNC_SYNC_STATISTICS_H_

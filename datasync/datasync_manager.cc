// Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "datasync/datasync_manager.h"

#include "datasync/datasync_instance.h"
#include "datasync/datasync_log.h"
#include "datasync/datasync_scoped_exit.h"
#include "datasync/sync_info.h"
#include "datasync/sync_service_info.h"
#include "datasync/sync_profile_info.h"

#include "tizen/tizen.h"

namespace {

const int MAX_PROFILES_NUM = 5;

sync_agent_ds_sync_mode_e ConvertToPlatformSyncMode(
    datasync::SyncInfo::SyncMode sync_mode) {
  switch (sync_mode) {
    case datasync::SyncInfo::MANUAL_MODE: {
      return SYNC_AGENT_SYNC_MODE_MANUAL;
    }
    case datasync::SyncInfo::PERIODIC_MODE: {
      return SYNC_AGENT_SYNC_MODE_PERIODIC;
    }
    case datasync::SyncInfo::PUSH_MODE: {
      return SYNC_AGENT_SYNC_MODE_PUSH;
    }
    default: {
      LogWarning("Error while converting a sync mode.");
      return SYNC_AGENT_SYNC_MODE_MANUAL;
    }
  }
}

datasync::SyncInfo::SyncMode ConvertToSyncMode(
    sync_agent_ds_sync_mode_e sync_mode) {
  switch (sync_mode) {
    case SYNC_AGENT_SYNC_MODE_MANUAL: {
      return datasync::SyncInfo::MANUAL_MODE;
    }
    case SYNC_AGENT_SYNC_MODE_PERIODIC: {
      return datasync::SyncInfo::PERIODIC_MODE;
    }
    case SYNC_AGENT_SYNC_MODE_PUSH: {
      return datasync::SyncInfo::PUSH_MODE;
    }
    default: {
      LogWarning("Error while converting a sync mode.");
      return datasync::SyncInfo::UNDEFINED_MODE;
    }
  }
}

sync_agent_ds_sync_type_e ConvertToPlatformSyncType(
    datasync::SyncInfo::SyncType syncType) {
  switch (syncType) {
    case datasync::SyncInfo::TWO_WAY_TYPE: {
      return SYNC_AGENT_SYNC_TYPE_UPDATE_BOTH;
    }
    case datasync::SyncInfo::SLOW_TYPE: {
      return SYNC_AGENT_SYNC_TYPE_FULL_SYNC;
    }
    case datasync::SyncInfo::ONE_WAY_FROM_CLIENT_TYPE: {
      return SYNC_AGENT_SYNC_TYPE_UPDATE_TO_SERVER;
    }
    case datasync::SyncInfo::REFRESH_FROM_CLIENT_TYPE: {
      return SYNC_AGENT_SYNC_TYPE_REFRESH_FROM_PHONE;
    }
    case datasync::SyncInfo::ONE_WAY_FROM_SERVER_TYPE: {
      return SYNC_AGENT_SYNC_TYPE_UPDATE_TO_PHONE;
    }
    case datasync::SyncInfo::REFRESH_FROM_SERVER_TYPE: {
      return SYNC_AGENT_SYNC_TYPE_REFRESH_FROM_SERVER;
    }
    default: {
      LogWarning("Error while converting a sync type.");
      return SYNC_AGENT_SYNC_TYPE_UPDATE_BOTH;
    }
  }
}

sync_agent_ds_src_uri_e ConvertToPlatformSourceUri(
    datasync::SyncServiceInfo::SyncServiceType serviceType) {
  switch (serviceType) {
    case datasync::SyncServiceInfo::CONTACT_SERVICE_TYPE: {
      return SYNC_AGENT_SRC_URI_CONTACT;
    }
    case datasync::SyncServiceInfo::EVENT_SERVICE_TYPE: {
      return SYNC_AGENT_SRC_URI_CALENDAR;
    }
    default: {
      LogWarning("Error while converting a sync service.");
      return SYNC_AGENT_SRC_URI_CONTACT;
    }
  }
}

datasync::SyncStatistics::SyncStatus ConvertToSyncStatus(char* status) {
  if (0 == strncmp(status, "success", 7)) {
    return datasync::SyncStatistics::SUCCESS_STATUS;
  } else if (0 == strncmp(status, "stop", 4)) {
    return datasync::SyncStatistics::STOP_STATUS;
  } else if (0 == strncmp(status, "fail", 4)) {
    return datasync::SyncStatistics::FAIL_STATUS;
  } else if (0 == strncmp(status, "No", 2)) {
    return datasync::SyncStatistics::NONE_STATUS;
  } else {
    LogWarning("Error while converting a sync status.");
  }

  return datasync::SyncStatistics::NONE_STATUS;
}

datasync::SyncInfo::SyncType ConvertToSyncType(
    sync_agent_ds_sync_type_e sync_type) {
  switch (sync_type) {
    case SYNC_AGENT_SYNC_TYPE_UPDATE_BOTH: {
      return datasync::SyncInfo::TWO_WAY_TYPE;
    }
    case SYNC_AGENT_SYNC_TYPE_FULL_SYNC: {
      return datasync::SyncInfo::SLOW_TYPE;
    }
    case SYNC_AGENT_SYNC_TYPE_UPDATE_TO_SERVER: {
      return datasync::SyncInfo::ONE_WAY_FROM_CLIENT_TYPE;
    }
    case SYNC_AGENT_SYNC_TYPE_REFRESH_FROM_PHONE: {
      return datasync::SyncInfo::REFRESH_FROM_CLIENT_TYPE;
    }
    case  SYNC_AGENT_SYNC_TYPE_UPDATE_TO_PHONE: {
      return datasync::SyncInfo::ONE_WAY_FROM_SERVER_TYPE;
    }
    case SYNC_AGENT_SYNC_TYPE_REFRESH_FROM_SERVER: {
      return datasync::SyncInfo::REFRESH_FROM_SERVER_TYPE;
    }
    default: {
      LogWarning("Error while converting a sync type.");
      return datasync::SyncInfo::UNDEFINED_TYPE;
    }
  }
}

sync_agent_ds_sync_interval_e ConvertToPlatformSyncInterval(
    datasync::SyncInfo::SyncInterval sync_interval) {
  switch (sync_interval) {
    case datasync::SyncInfo::INTERVAL_5_MINUTES: {
      return SYNC_AGENT_SYNC_INTERVAL_5_MINUTES;
    }
    case datasync::SyncInfo::INTERVAL_15_MINUTES: {
      return SYNC_AGENT_SYNC_INTERVAL_15_MINUTES;
    }
    case datasync::SyncInfo::INTERVAL_1_HOUR: {
      return SYNC_AGENT_SYNC_INTERVAL_1_HOUR;
    }
    case datasync::SyncInfo::INTERVAL_4_HOURS: {
      return SYNC_AGENT_SYNC_INTERVAL_4_HOURS;
    }
    case datasync::SyncInfo::INTERVAL_12_HOURS: {
      return SYNC_AGENT_SYNC_INTERVAL_12_HOURS;
    }
    case datasync::SyncInfo::INTERVAL_1_DAY: {
      return SYNC_AGENT_SYNC_INTERVAL_1_DAY;
    }
    case datasync::SyncInfo::INTERVAL_1_WEEK: {
      return SYNC_AGENT_SYNC_INTERVAL_1_WEEK;
    }
    case datasync::SyncInfo::INTERVAL_1_MONTH: {
      return SYNC_AGENT_SYNC_INTERVAL_1_MONTH;
    }
    case datasync::SyncInfo::INTERVAL_UNDEFINED: {
      return SYNC_AGENT_SYNC_INTERVAL_NONE;
    }
    default: {
      LogWarning("Error while converting a JS sync interval.");
      return SYNC_AGENT_SYNC_INTERVAL_1_WEEK;
    }
  }
}

datasync::SyncInfo::SyncInterval ConvertToSyncInterval(
    sync_agent_ds_sync_interval_e sync_interval) {
  switch (sync_interval) {
    case SYNC_AGENT_SYNC_INTERVAL_5_MINUTES: {
      return datasync::SyncInfo::INTERVAL_5_MINUTES;
    }
    case SYNC_AGENT_SYNC_INTERVAL_15_MINUTES: {
      return datasync::SyncInfo::INTERVAL_15_MINUTES;
    }
    case SYNC_AGENT_SYNC_INTERVAL_1_HOUR: {
      return datasync::SyncInfo::INTERVAL_1_HOUR;
    }
    case SYNC_AGENT_SYNC_INTERVAL_4_HOURS: {
      return datasync::SyncInfo::INTERVAL_4_HOURS;
    }
    case SYNC_AGENT_SYNC_INTERVAL_12_HOURS: {
      return datasync::SyncInfo::INTERVAL_12_HOURS;
    }
    case SYNC_AGENT_SYNC_INTERVAL_1_DAY: {
      return datasync::SyncInfo::INTERVAL_1_DAY;
    }
    case SYNC_AGENT_SYNC_INTERVAL_1_WEEK: {
      return datasync::SyncInfo::INTERVAL_1_WEEK;
    }
    case SYNC_AGENT_SYNC_INTERVAL_1_MONTH: {
      return datasync::SyncInfo::INTERVAL_1_MONTH;
    }
    case SYNC_AGENT_SYNC_INTERVAL_NONE: {
      return datasync::SyncInfo::INTERVAL_UNDEFINED;
    }
    default: {
      LogWarning("Error while converting a platform sync interval.");
      return datasync::SyncInfo::INTERVAL_UNDEFINED;
    }
  }
}

sync_agent_ds_service_type_e ConvertToPlatformSyncServiceType(
    datasync::SyncServiceInfo::SyncServiceType service_type) {
  switch (service_type) {
    case datasync::SyncServiceInfo::CONTACT_SERVICE_TYPE: {
      return SYNC_AGENT_CONTACT;
    }
    case datasync::SyncServiceInfo::EVENT_SERVICE_TYPE: {
      return SYNC_AGENT_CALENDAR;
    }
    default: {
      LogWarning("Error while converting a sync service type.");
      return SYNC_AGENT_CONTACT;
    }
  }
}

datasync::SyncServiceInfo::SyncServiceType ConvertToSyncServiceType(
    sync_agent_ds_service_type_e service_type) {
  switch (service_type) {
    case SYNC_AGENT_CONTACT: {
      return datasync::SyncServiceInfo::CONTACT_SERVICE_TYPE;
    }
    case SYNC_AGENT_CALENDAR: {
      return datasync::SyncServiceInfo::EVENT_SERVICE_TYPE;
    }
    default: {
      LogWarning("Error while converting a sync service type.");
      return datasync::SyncServiceInfo::UNDEFINED_SERVICE_TYPE;
    }
  }
}

}  // namespace

namespace datasync {

bool DataSyncManager::sync_agent_initialized_ = false;

DataSyncManager::DataSyncManager(DatasyncInstance& parent)
    : instance_(parent) {
  // initialize sync agent once per process
  if (sync_agent_initialized_) {
    return;
  }

  LogInfo("Initialize the datasync manager");
  sync_agent_ds_error_e ds_err = sync_agent_ds_init();
  if (SYNC_AGENT_DS_SUCCESS != ds_err) {
    LogError("Failed to init oma ds.");
  } else {
    sync_agent_initialized_ = true;
  }
}

DataSyncManager::~DataSyncManager() {
// TODO(t.iwanek): sync-agent crashes internally..
// sync-agent should fix it's deinitialization
}

ResultOrError<std::string> DataSyncManager::Add(SyncProfileInfo& profile_info) {
  ds_profile_h profile_h = nullptr;
  char* profile_name = nullptr;

  // Check if the quota is full first.
  GList* profile_list = nullptr;
  GList* iter = nullptr;

  ScopedExit exit([&profile_name, &profile_h](){
    if (profile_name) {
      free(profile_name);
    }

    if (profile_h) {
      sync_agent_ds_free_profile_info(profile_h);
    }
  });

  sync_agent_ds_error_e ret = SYNC_AGENT_DS_FAIL;

  ret = sync_agent_ds_get_all_profile(&profile_list);
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("Exception",
        "Platform error while getting all profiles");
  }

  int num_profiles = g_list_length(profile_list);
  for (iter = profile_list; iter != nullptr; iter = g_list_next(iter)) {
    sync_agent_ds_free_profile_info((ds_profile_h)iter->data);
  }
  if (profile_list) {
    g_list_free(profile_list);
  }
  LogDebug("numProfiles: " << num_profiles);
  if (MAX_PROFILES_NUM == num_profiles) {
    return Error("OutOfRangeException",
        "There are already maximum number of profiles!");
  }

  ret = sync_agent_ds_create_profile_info(&profile_h);
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("Exception",
        "Platform error while creating a profile");
  }

  ret = sync_agent_ds_set_profile_name(
      profile_h, const_cast<char*>(profile_info.profile_name().c_str()));
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("Exception",
        "Platform error while settting a profile name");
  }

  ret = sync_agent_ds_set_server_info(
      profile_h, const_cast<char*>(profile_info.sync_info()->url().c_str()),
      const_cast<char*>(profile_info.sync_info()->id().c_str()),
      const_cast<char*>(profile_info.sync_info()->password().c_str()));
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("Exception",
        "Platform error while settting a server info");
  }

  sync_agent_ds_sync_mode_e sync_mode =
      ConvertToPlatformSyncMode(profile_info.sync_info()->sync_mode());
  sync_agent_ds_sync_type_e sync_type =
      ConvertToPlatformSyncType(profile_info.sync_info()->sync_type());
  sync_agent_ds_sync_interval_e sync_interval =
      ConvertToPlatformSyncInterval(profile_info.sync_info()->sync_interval());
  LogDebug("syncMode: " << sync_mode << ", syncType: " << sync_type
                        << ", syncInterval: " << sync_interval);

  ret = sync_agent_ds_set_sync_info(profile_h, sync_mode, sync_type,
                                    sync_interval);
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("Exception",
        "Platform error while settting a sync info");
  }

  // Set the sync categories.
  SyncServiceInfoListPtr categories = profile_info.service_info();
  for (unsigned int i = 0; categories->size() < i; ++i) {
    sync_agent_ds_service_type_e service_type =
        ConvertToPlatformSyncServiceType(
            categories->at(i)->sync_service_type());
    std::string tgt_uri = categories->at(i)->server_database_uri();
    sync_agent_ds_src_uri_e src_uri =
        ConvertToPlatformSourceUri(categories->at(i)->sync_service_type());
    std::string id = categories->at(i)->id();
    std::string password = categories->at(i)->password();
    bool enable = categories->at(i)->enable();

    LogInfo("serviceType: " << service_type << ", tgtURI: " << tgt_uri
                            << ", enable: " << enable << " for index: " << i);

    ret = sync_agent_ds_set_sync_service_info(
        profile_h, service_type, enable, src_uri,
        const_cast<char*>(tgt_uri.c_str()),
        0 == id.size() ? nullptr : const_cast<char*>(id.c_str()),
        0 == password.size() ? nullptr : const_cast<char*>(password.c_str()));
    if (SYNC_AGENT_DS_SUCCESS != ret) {
      return Error("Exception",
          "Platform error while settting a sync service info");
    }
  }

  int profile_id;
  ret = sync_agent_ds_add_profile(profile_h, &profile_id);
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("Exception",
        "Platform error while adding a profile");
  }

  LogDebug("profileId from platform: " << profile_id);

  ret = sync_agent_ds_get_profile_name(profile_h, &profile_name);
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("Exception",
        "Platform error while getting a profile name");
  }

  LogDebug("profileName: " << profile_name << ", profileId: " << profile_id);

  profile_info.set_profile_id(std::to_string(profile_id));

  return profile_info.profile_id();
}

ResultOrError<void> DataSyncManager::Update(SyncProfileInfo& profile_info) {
  ds_profile_h profile_h = nullptr;

  ScopedExit exit([&profile_h]() {
    if (profile_h) {
      sync_agent_ds_free_profile_info(profile_h);
    }
  });

  sync_agent_ds_error_e ret = SYNC_AGENT_DS_FAIL;

  int profile_id = std::stoi(profile_info.profile_id());
  LogDebug("profileId: " << profile_id);

  ret = sync_agent_ds_get_profile(profile_id, &profile_h);
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("NotFoundException",
        "Platform error while getting a profile");
  }

  ret = sync_agent_ds_set_profile_name(
      profile_h, const_cast<char*>(profile_info.profile_name().c_str()));
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("Exception",
        "Platform error while settting a profile name");
  }

  ret = sync_agent_ds_set_server_info(
      profile_h, const_cast<char*>(profile_info.sync_info()->url().c_str()),
      const_cast<char*>(profile_info.sync_info()->id().c_str()),
      const_cast<char*>(profile_info.sync_info()->password().c_str()));
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("Exception",
        "Platform error while settting a server info");
  }

  sync_agent_ds_sync_mode_e sync_mode =
      ConvertToPlatformSyncMode(profile_info.sync_info()->sync_mode());
  sync_agent_ds_sync_type_e sync_type =
      ConvertToPlatformSyncType(profile_info.sync_info()->sync_type());
  sync_agent_ds_sync_interval_e sync_interval =
      ConvertToPlatformSyncInterval(profile_info.sync_info()->sync_interval());
  LogDebug("syncMode: " << sync_mode << ", syncType: " << sync_type
                        << ", syncInterval: " << sync_interval);

  ret = sync_agent_ds_set_sync_info(profile_h, sync_mode, sync_type,
                                    sync_interval);
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("Exception",
        "Platform error while settting a sync info");
  }

  // Set the sync categories.
  SyncServiceInfoListPtr categories = profile_info.service_info();
  for (unsigned int i = 0; categories->size() < i; ++i) {
    sync_agent_ds_service_type_e service_type =
        ConvertToPlatformSyncServiceType(
            categories->at(i)->sync_service_type());
    std::string tgt_uri = categories->at(i)->server_database_uri();
    sync_agent_ds_src_uri_e src_uri =
        ConvertToPlatformSourceUri(categories->at(i)->sync_service_type());
    std::string id = categories->at(i)->id();
    std::string password = categories->at(i)->password();
    bool enable = categories->at(i)->enable();

    LogDebug("serviceType: " << service_type << ", tgtURI: " << tgt_uri
                             << " for index: " << i);

    ret = sync_agent_ds_set_sync_service_info(
        profile_h, service_type, enable, src_uri,
        const_cast<char*>(tgt_uri.c_str()),
        0 == id.size() ? nullptr : const_cast<char*>(id.c_str()),
        0 == password.size() ? nullptr : const_cast<char*>(password.c_str()));
    if (SYNC_AGENT_DS_SUCCESS != ret) {
      return Error("Exception",
          "Platform error while settting a sync service info");
    }
  }

  ret = sync_agent_ds_update_profile(profile_h);
  if (SYNC_AGENT_DS_SUCCESS != ret && SYNC_AGENT_DS_SYNCHRONISING != ret) {
    return Error("Exception",
        "Platform error while updating a profile");
  }
  return {};
}

ResultOrError<void> DataSyncManager::Remove(const std::string& id) {
  ds_profile_h profile_h = nullptr;

  ScopedExit exit([&profile_h]() {
    if (profile_h) {
      sync_agent_ds_free_profile_info(profile_h);
    }
  });

  sync_agent_ds_error_e ret = SYNC_AGENT_DS_FAIL;

  int profile_id = std::stoi(id);
  LogDebug("profileId: " << profile_id);

  ret = sync_agent_ds_get_profile(profile_id, &profile_h);
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("Exception",
        "Platform error while getting a profile");
  }

  ret = sync_agent_ds_delete_profile(profile_h);
  if (SYNC_AGENT_DS_SUCCESS != ret && SYNC_AGENT_DS_SYNCHRONISING != ret) {
    return Error("Exception",
        "Platform error while deleting a profile");
  }

  return {};
}

ResultOrError<unsigned> DataSyncManager::GetMaxProfilesNum() const {
  return MAX_PROFILES_NUM;
}

ResultOrError<unsigned> DataSyncManager::GetProfilesNum() const {
  GList* profile_list = nullptr;
  GList* iter = nullptr;

  sync_agent_ds_error_e ret = SYNC_AGENT_DS_FAIL;

  ret = sync_agent_ds_get_all_profile(&profile_list);
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("Exception",
        "Platform error while getting all profiles");
  }

  int num_profiles = 0;
  for (iter = profile_list; iter != nullptr; iter = g_list_next(iter)) {
    sync_agent_ds_free_profile_info((ds_profile_h)iter->data);
    num_profiles++;
    LogDebug("Free sync_agent_ds_profile_info for index: " << num_profiles);
  }

  LogDebug("numProfiles: " << num_profiles);

  return num_profiles;
}

ResultOrError<SyncProfileInfoPtr> DataSyncManager::Get(
    const std::string& profile_id) const {
  ds_profile_h profile_h = nullptr;
  GList* category_list = nullptr;

  ScopedExit exit([&category_list, &profile_h]() {
    if (category_list) {
      g_list_free(category_list);
    }

    if (profile_h) {
      sync_agent_ds_free_profile_info(profile_h);
    }
  });

  SyncProfileInfoPtr profile(new SyncProfileInfo());

  sync_agent_ds_error_e ret = SYNC_AGENT_DS_FAIL;

  int profile_id_str = std::stoi(profile_id);
  LogDebug("profileId: " << profile_id_str);

  ret = sync_agent_ds_get_profile(profile_id_str, &profile_h);
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("NotFoundException",
        "Platform error while getting a profile");
  }

  profile->set_profile_id(profile_id);

  char* profile_name = nullptr;
  ret = sync_agent_ds_get_profile_name(profile_h, &profile_name);
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("Exception",
        "Platform error while gettting a profile name");
  }
  profile->set_profile_name(profile_name);

  sync_agent_ds_server_info server_info = {nullptr};
  ret = sync_agent_ds_get_server_info(profile_h, &server_info);
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("Exception",
        "Platform error while gettting a server info");
  }
  profile->sync_info()->set_url(server_info.addr);
  profile->sync_info()->set_id(server_info.id);
  profile->sync_info()->set_password(server_info.password);

  sync_agent_ds_sync_info sync_info;
  ret = sync_agent_ds_get_sync_info(profile_h, &sync_info);
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("Exception",
        "Platform error while gettting a sync info");
  }
  profile->sync_info()->set_sync_mode(ConvertToSyncMode(sync_info.sync_mode));
  profile->sync_info()->set_sync_type(ConvertToSyncType(sync_info.sync_type));
  profile->sync_info()->set_sync_interval(
      ConvertToSyncInterval(sync_info.interval));

  LogDebug("Sync mode: " << sync_info.sync_mode
                         << ", type: " << sync_info.sync_type
                         << ", interval: " << sync_info.interval);

  sync_agent_ds_service_info* category_info = nullptr;
  ret = sync_agent_ds_get_sync_service_info(profile_h, &category_list);
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("Exception",
        "Platform error while gettting sync categories");
  }
  int category_count = g_list_length(category_list);
  LogDebug("category_count: " << category_count);
  while (category_count--) {
    category_info = static_cast<sync_agent_ds_service_info*>(
        g_list_nth_data(category_list, category_count));
    if (SYNC_AGENT_CALENDAR < category_info->service_type) {
      LogDebug("Skip unsupported sync service type: "
               << category_info->service_type);
      continue;
    }

    SyncServiceInfoPtr service_info(new SyncServiceInfo());
    service_info->set_enable(category_info->enabled);
    if (category_info->id) {
      service_info->set_id(category_info->id);
    }
    if (category_info->password) {
      service_info->set_password(category_info->password);
    }
    service_info->set_sync_service_type(
        ConvertToSyncServiceType(category_info->service_type));
    if (category_info->tgt_uri) {
      service_info->set_server_database_uri(category_info->tgt_uri);
    }

    LogDebug("Service type: " << service_info->sync_service_type());
    profile->service_info()->push_back(service_info);
  }

  return profile;
}

ResultOrError<SyncProfileInfoListPtr> DataSyncManager::GetAll() const {
  GList* profile_list = nullptr;
  GList* iter = nullptr;

  ds_profile_h profile_h = nullptr;

  ScopedExit exit([&profile_list, &profile_h, &iter]() {
    LogDebug("Free profiles list.");
    for (iter = profile_list; iter != nullptr; iter = g_list_next(iter)) {
      sync_agent_ds_free_profile_info((ds_profile_h)iter->data);
    }

    if (profile_list) {
      g_list_free(profile_list);
    }
  });

  SyncProfileInfoListPtr profiles = std::make_shared<SyncProfileInfoList>();

  sync_agent_ds_error_e ret = SYNC_AGENT_DS_FAIL;

  ret = sync_agent_ds_get_all_profile(&profile_list);
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("Exception",
        "Platform error while getting all profiles");
  }

  int profile_id;
  LogDebug("Number of profiles: " << g_list_length(profile_list));
  for (iter = profile_list; iter != nullptr; iter = g_list_next(iter)) {
    profile_h = (ds_profile_h)iter->data;
    SyncProfileInfoPtr profile(new SyncProfileInfo());

    ret = sync_agent_ds_get_profile_id(profile_h, &profile_id);
    if (SYNC_AGENT_DS_SUCCESS != ret) {
      return Error("Exception",
          "Platform error while gettting a profile id");
    }

    profile->set_profile_id(std::to_string(profile_id));

    LogDebug("Processing a profile with id: " << profile->profile_id());

    char* profile_name = nullptr;
    ret = sync_agent_ds_get_profile_name(profile_h, &profile_name);
    if (SYNC_AGENT_DS_SUCCESS != ret) {
      return Error("Exception",
          "Platform error while gettting a profile name");
    }
    profile->set_profile_name(profile_name);

    sync_agent_ds_server_info server_info = {nullptr};
    ret = sync_agent_ds_get_server_info(profile_h, &server_info);
    if (SYNC_AGENT_DS_SUCCESS != ret) {
      return Error("Exception",
          "Platform error while gettting a server info");
    }
    profile->sync_info()->set_url(server_info.addr);
    profile->sync_info()->set_id(server_info.id);
    profile->sync_info()->set_password(server_info.password);

    sync_agent_ds_sync_info sync_info;
    ret = sync_agent_ds_get_sync_info(profile_h, &sync_info);
    if (SYNC_AGENT_DS_SUCCESS != ret) {
      return Error("Exception",
          "Platform error while gettting a sync info");
    }
    profile->sync_info()->set_sync_mode(
        ConvertToSyncMode(sync_info.sync_mode));
    profile->sync_info()->set_sync_type(
        ConvertToSyncType(sync_info.sync_type));
    profile->sync_info()->set_sync_interval(
        ConvertToSyncInterval(sync_info.interval));

    LogDebug("Sync mode: " << sync_info.sync_mode
                           << ", type: " << sync_info.sync_type
                           << ", interval: " << sync_info.interval);

    GList* category_list = nullptr;
    sync_agent_ds_service_info* category_info = nullptr;
    ret = sync_agent_ds_get_sync_service_info(profile_h, &category_list);
    if (SYNC_AGENT_DS_SUCCESS != ret) {
      return Error(
          "Exception",
          "Platform error while gettting sync categories");
    }
    int category_count = g_list_length(category_list);
    LogDebug("category_count: " << category_count);
    while (category_count--) {
      category_info = static_cast<sync_agent_ds_service_info*>(
          g_list_nth_data(category_list, category_count));
      if (SYNC_AGENT_CALENDAR < category_info->service_type) {
        LogDebug("Skip unsupported sync service type: "
            << category_info->service_type);
        continue;
      }

      SyncServiceInfoPtr service_info(new SyncServiceInfo());
      service_info->set_enable(category_info->enabled);
      if (category_info->id) {
        service_info->set_id(category_info->id);
      }
      if (category_info->password) {
        service_info->set_password(category_info->password);
      }
      service_info->set_sync_service_type(
          ConvertToSyncServiceType(category_info->service_type));
      if (category_info->tgt_uri) {
        service_info->set_server_database_uri(category_info->tgt_uri);
      }

      LogDebug("Service type: " << service_info->sync_service_type());
      profile->service_info()->push_back(service_info);
    }
    if (category_list) {
      g_list_free(category_list);
    }

    LogDebug("Adding a profile to the list.");
    profiles->push_back(profile);
  }

  return profiles;
}

ResultOrError<void> DataSyncManager::StartSync(
    const std::string& profile_id_str, int callback_id) {
  ds_profile_h profile_h = nullptr;

  ScopedExit exit([&profile_h]() {
    if (profile_h) {
      sync_agent_ds_free_profile_info(profile_h);
    }
  });

  sync_agent_ds_error_e ret = SYNC_AGENT_DS_FAIL;
  sync_agent_event_error_e err = SYNC_AGENT_EVENT_FAIL;

  int profile_id = std::stoi(profile_id_str);
  LogDebug("profileId: " << profile_id);

  ret = sync_agent_ds_get_profile(profile_id, &profile_h);
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("Exception",
        "Platform error while getting a profile");
  }

  err = sync_agent_set_noti_callback(
      1, [](sync_agent_event_data_s* d, void* ud) {
           return static_cast<DataSyncManager*>(ud)->StateChangedCallback(d);
         },
      static_cast<void*>(this));
  if (SYNC_AGENT_EVENT_SUCCESS != err) {
    return Error("Exception",
        "Platform error while setting state changed cb");
  }

  err = sync_agent_set_noti_callback(
      2, [](sync_agent_event_data_s* d, void* ud) {
           return static_cast<DataSyncManager*>(ud)->ProgressCallback(d);
         },
      static_cast<void*>(this));
  if (SYNC_AGENT_EVENT_SUCCESS != err) {
    return Error("Exception",
        "Platform error while setting progress cb");
  }

  ret = sync_agent_ds_start_sync(profile_h);
  if (SYNC_AGENT_DS_SUCCESS != ret && SYNC_AGENT_DS_SYNCHRONISING != ret) {
    return Error("Exception",
        "Platform error while starting a profile");
  }

  if (callback_id >= 0) {
    callbacks_.insert({profile_id, callback_id});
  }

  return {};
}

ResultOrError<void> DataSyncManager::StopSync(
    const std::string& profile_id_str) {
  ds_profile_h profile_h = nullptr;

  ScopedExit exit([&profile_h]() {
    if (profile_h) {
      sync_agent_ds_free_profile_info(profile_h);
    }
  });

  sync_agent_ds_error_e ret = SYNC_AGENT_DS_FAIL;

  int profile_id = std::stoi(profile_id_str.c_str());
  LogDebug("profileId: " << profile_id);

  ret = sync_agent_ds_get_profile(profile_id, &profile_h);
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("Exception",
        "Platform error while getting a profile");
  }

  ret = sync_agent_ds_stop_sync(profile_h);
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("Exception",
        "Platform error while stopping a profile");
  }

  return {};
}

ResultOrError<SyncStatisticsListPtr> DataSyncManager::GetLastSyncStatistics(
    const std::string& profile_str_id) const {
  ds_profile_h profile_h = nullptr;
  GList* statistics_list = nullptr;

  ScopedExit exit([&statistics_list, &profile_h]() {
    if (statistics_list) {
      g_list_free(statistics_list);
    }

    if (profile_h) {
      sync_agent_ds_free_profile_info(profile_h);
    }
  });

  SyncStatisticsListPtr statistics_list_ptr(new SyncStatisticsList());

  sync_agent_ds_error_e ret = SYNC_AGENT_DS_FAIL;

  int profile_id = std::stoi(profile_str_id);
  LogDebug("profileId: " << profile_id);
  ret = sync_agent_ds_get_profile(profile_id, &profile_h);
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("NotFoundException",
                            "Platform error while getting a profile");
  }

  ret = sync_agent_ds_get_sync_statistics(profile_h, &statistics_list);
  if (SYNC_AGENT_DS_SUCCESS != ret) {
    return Error("Exception",
                            "Platform error while gettting sync statistics");
  }

  int statistics_count = g_list_length(statistics_list);
  LogDebug("statistics_count: " << statistics_count);
  sync_agent_ds_statistics_info* statistics = nullptr;
  for (int i = 0; i < statistics_count; i++) {
    statistics = static_cast<sync_agent_ds_statistics_info*>(
        g_list_nth_data(statistics_list, i));

    SyncStatisticsPtr statistics_ptr(new SyncStatistics());

    if (0 == i) {
      LogDebug("Statistics for contact.");
      statistics_ptr->set_service_type(
          ConvertToSyncServiceType(SYNC_AGENT_CONTACT));
    } else if (1 == i) {
      LogDebug("Statistics for event.");
      statistics_ptr->set_service_type(
          ConvertToSyncServiceType(SYNC_AGENT_CALENDAR));
    } else {
      LogWarning("Unsupported category for statistics: " << i);
      continue;
    }

    LogDebug("dbsynced: " << statistics->dbsynced);
    if (statistics->dbsynced) {
      statistics_ptr->set_sync_status(
          ConvertToSyncStatus(statistics->dbsynced));
      statistics_ptr->set_client_to_server_total(
          statistics->client2server_total);
      statistics_ptr->set_client_to_server_added(
          statistics->client2server_nrofadd);
      statistics_ptr->set_client_to_server_updated(
          statistics->client2server_nrofreplace);
      statistics_ptr->set_client_to_server_removed(
          statistics->client2server_nrofdelete);
      statistics_ptr->set_server_to_client_total(
          statistics->server2client_total);
      statistics_ptr->set_server_to_client_added(
          statistics->server2client_nrofadd);
      statistics_ptr->set_server_to_client_updated(
          statistics->server2client_nrofreplace);
      statistics_ptr->set_server_to_client_removed(
          statistics->server2client_nrofdelete);

      statistics_ptr->set_last_sync_time(
          static_cast<unsigned>(statistics->last_session_time));
    }

    LogDebug(
        "ClientToServerTotal: " << statistics_ptr->client_to_server_total()
                                << ", ServerToClientTotal: "
                                << statistics_ptr->server_to_client_total());

    statistics_list_ptr->push_back(statistics_ptr);
  }

  return statistics_list_ptr;
}

int DataSyncManager::StateChangedCallback(sync_agent_event_data_s* request) {
  LogDebug("DataSync session state changed.");

  char* profile_dir_name = nullptr;
  int sync_type = 0;
  char* progress = nullptr;
  char* error = nullptr;

  LogDebug("Get state info.");
  sync_agent_get_event_data_param(request, &profile_dir_name);
  sync_agent_get_event_data_param(request, &sync_type);
  sync_agent_get_event_data_param(request, &progress);
  sync_agent_get_event_data_param(request, &error);

  LogInfo("profileDirName: " << profile_dir_name
                             << ", sync_type: " << sync_type
                             << ", progress: " << progress
                             << ", error: " << error);

  if (profile_dir_name) {
    std::string profileDirNameStr(profile_dir_name);

    // truncate the rest
    profileDirNameStr.resize(4);
    int profileId = std::stoi(profileDirNameStr);

    auto it = callbacks_.find(profileId);
    if (it != callbacks_.end()) {
      int callback_id = it->second;
      callbacks_.erase(it);

      if (nullptr == progress) {
        LogWarning("nullptr status.");
        instance_.ReplyAsyncOnFailed(callback_id, profileDirNameStr,
                                     UNKNOWN_ERR, "Exception",
                                     "nullptr status");
      } else if (0 == strncmp(progress, "DONE", 4)) {
        instance_.ReplyAsyncOnCompleted(callback_id, profileDirNameStr);
      } else if (0 == strncmp(progress, "CANCEL", 6)) {
        instance_.ReplyAsyncOnStopped(callback_id, profileDirNameStr);
      } else if (0 == strncmp(progress, "ERROR", 5)) {
        instance_.ReplyAsyncOnFailed(callback_id, profileDirNameStr,
                                     UNKNOWN_ERR, "Exception",
                                     "Datasync failed");
      } else {
        LogInfo("Undefined status");
        instance_.ReplyAsyncOnFailed(callback_id, profileDirNameStr,
                                     UNKNOWN_ERR, "Exception",
                                     "Undefined status");
      }
    }
  }

  g_free(profile_dir_name);
  g_free(progress);
  g_free(error);

  if (request->size != nullptr) {
    g_free(request->size);
  }
  g_free(request);

  return 0;
}

int DataSyncManager::ProgressCallback(sync_agent_event_data_s* request) {
  LogDebug("DataSync progress called.");

  char* profile_dir_name = nullptr;
  int sync_type = 0;
  int uri;
  char* progress_status = nullptr;
  char* operation_type = nullptr;

  int is_from_server, total_per_operation, synced_per_operation, total_per_db,
      synced_per_db;

  LogDebug("Get progress info.");
  sync_agent_get_event_data_param(request, &profile_dir_name);
  sync_agent_get_event_data_param(request, &sync_type);
  sync_agent_get_event_data_param(request, &uri);
  sync_agent_get_event_data_param(request, &progress_status);
  sync_agent_get_event_data_param(request, &operation_type);

  LogInfo("profileDirName: " << profile_dir_name << ", syncType: " << sync_type
                             << ", uri: " << uri
                             << ", progressStatus: " << progress_status
                             << ", operationType " << operation_type);

  sync_agent_get_event_data_param(request, &is_from_server);
  sync_agent_get_event_data_param(request, &total_per_operation);
  sync_agent_get_event_data_param(request, &synced_per_operation);
  sync_agent_get_event_data_param(request, &total_per_db);
  sync_agent_get_event_data_param(request, &synced_per_db);

  LogInfo("isFromServer: " << is_from_server
                           << ", totalPerOperation: " << total_per_operation
                           << ", syncedPerOperation: " << synced_per_operation
                           << ", totalPerDb: " << total_per_db
                           << ", syncedPerDb " << synced_per_db);

  if (profile_dir_name) {
    std::string profile_dir_name_str(profile_dir_name);
    profile_dir_name_str.resize(4);
    int profile_id = std::stoi(profile_dir_name_str);

    auto it = callbacks_.find(profile_id);
    if (it != callbacks_.end()) {
      int callback_id = it->second;

      if (SYNC_AGENT_SRC_URI_CONTACT == uri) {
        instance_.ReplyAsyncOnProgress(callback_id, profile_dir_name_str,
                                       is_from_server, synced_per_db,
                                       total_per_db,
                                       SyncServiceInfo::CONTACT_SERVICE_TYPE);
      } else if (SYNC_AGENT_SRC_URI_CALENDAR == uri) {
        instance_.ReplyAsyncOnProgress(callback_id, profile_dir_name_str,
                                       is_from_server, synced_per_db,
                                       total_per_db,
                                       SyncServiceInfo::EVENT_SERVICE_TYPE);
      } else {
        LogWarning("Wrong service type");
        instance_.ReplyAsyncOnFailed(callback_id, profile_dir_name_str,
                                     UNKNOWN_ERR, "Exception",
                                     "Wrong service type");
      }
    }
  }

  g_free(profile_dir_name);
  g_free(progress_status);
  g_free(operation_type);

  if (request != nullptr) {
    if (request->size != nullptr) {
      g_free(request->size);
    }
    g_free(request);
  }

  return 0;
}

}  // namespace datasync

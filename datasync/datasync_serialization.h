// Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DATASYNC_DATASYNC_SERIALIZATION_H_
#define DATASYNC_DATASYNC_SERIALIZATION_H_

#include <functional>
#include <memory>
#include <string>

#include "common/picojson.h"

#include "datasync/sync_info.h"
#include "datasync/sync_profile_info.h"
#include "datasync/sync_service_info.h"
#include "datasync/sync_statistics.h"

namespace datasync {
namespace serialization {
namespace detail {

enum class ConvResult {
  SUCCESS,
  MISSING,
  ERROR
};

// picojson does not accepts int in value construction
template <class Type>
struct ConvertToPicojsonType {
  typedef Type AccessType;
  typedef Type ConstructType;
};

template <>
struct ConvertToPicojsonType<unsigned> {
  typedef int AccessType;
  typedef double ConstructType;
};

template <>
struct ConvertToPicojsonType<int> {
  typedef int AccessType;
  typedef double ConstructType;
};

/**
 * Gets member 'field' field for given 'obj' object.
 *
 * Template parameter T of type of field to be extracted.
 */
template <class T>
ConvResult GetMember(const picojson::object& obj, const char* field, T* out) {
  const auto& it = obj.find(field);
  if (it != obj.end()) {
    if (it->second.is<typename ConvertToPicojsonType<T>::AccessType>()) {
      *out = it->second.get<typename ConvertToPicojsonType<T>::AccessType>();
      return ConvResult::SUCCESS;
    } else {
      return ConvResult::ERROR;
    }
  } else {
    return ConvResult::MISSING;
  }
}

/**
 * Gets member 'field' field for given 'obj' object and converts value to
 * expected type T using adapter functor.
 *
 * Template parameter T states for type of target type of extracted field
 * Template parameter A states for type of type of extracted field received
 * from picojson.
 * Template parameter Adapter states for functor type converting A to T type
 */
template <class T, class A, class Adapter>
ConvResult GetMember(const picojson::object& obj, const char* field,
    const Adapter& adapter, T* out) {
  const auto& it = obj.find(field);
  if (it != obj.end()) {
    const auto& fld = it->second;
    if (fld.is<typename ConvertToPicojsonType<A>::AccessType>()) {
      *out = adapter(fld.get<typename ConvertToPicojsonType<A>::AccessType>());
      return ConvResult::SUCCESS;
    } else {
      return ConvResult::ERROR;
    }
  } else {
    return ConvResult::MISSING;
  }
}

}  // namespace detail

template <class T>
struct SerializationDefinition {
  static picojson::value ToJson(const T& type) {
    return picojson::value(
        static_cast<typename detail::ConvertToPicojsonType<T>::ConstructType>(
            type));
  }

  static std::unique_ptr<T> FromJson(const picojson::value& value) {
    if (value.is<typename detail::ConvertToPicojsonType<T>::AccessType>()) {
      return std::unique_ptr<T>(new T(
          value.get<typename detail::ConvertToPicojsonType<T>::AccessType>()));
    } else {
      return nullptr;
    }
  }
};

template <>
struct SerializationDefinition<SyncStatistics> {
  static picojson::value ToJson(const SyncStatistics& type) {
    picojson::object obj;
    obj["syncStatus"] = picojson::value(
        SyncStatistics::SyncStatusToString(type.sync_status()));
    obj["serviceType"] = picojson::value(
        SyncServiceInfo::SyncServiceTypeToString(type.service_type()));
    obj["lastSyncTime"] =
        picojson::value(static_cast<double>(type.last_sync_time()));

    obj["serverToClientTotal"] =
        picojson::value(static_cast<double>(type.server_to_client_total()));
    obj["serverToClientAdded"] =
        picojson::value(static_cast<double>(type.server_to_client_added()));
    obj["serverToClientUpdated"] =
        picojson::value(static_cast<double>(type.server_to_client_updated()));
    obj["serverToClientRemoved"] =
        picojson::value(static_cast<double>(type.server_to_client_removed()));

    obj["clientToServerTotal"] =
        picojson::value(static_cast<double>(type.client_to_server_total()));
    obj["clientToServerAdded"] =
        picojson::value(static_cast<double>(type.client_to_server_added()));
    obj["clientToServerUpdated"] =
        picojson::value(static_cast<double>(type.client_to_server_updated()));
    obj["clientToServerRemoved"] =
        picojson::value(static_cast<double>(type.client_to_server_removed()));

    return picojson::value(obj);
  }

  static std::unique_ptr<SyncStatistics> FromJson(
      const picojson::value& value) {
    std::unique_ptr<SyncStatistics> result;

    if (!value.is<picojson::object>()) {
      return nullptr;
    }
    const picojson::object& obj = value.get<picojson::object>();

    SyncStatistics::SyncStatus sync_status;
    if (detail::GetMember<SyncStatistics::SyncStatus, std::string>(
        obj, "syncStatus",
        std::bind(SyncStatistics::ConvertToSyncStatus, std::placeholders::_1),
        &sync_status)
        != detail::ConvResult::SUCCESS)
      return nullptr;

    SyncServiceInfo::SyncServiceType sync_service_type;
    if (detail::GetMember<SyncServiceInfo::SyncServiceType, std::string>(
        obj, "syncServiceType",
        std::bind(SyncServiceInfo::ConvertToSyncServiceType,
            std::placeholders::_1),
        &sync_service_type) != detail::ConvResult::SUCCESS)
      return nullptr;

    unsigned last_sync_time;
    if (detail::GetMember(obj, "lastSyncTime",
        &last_sync_time) != detail::ConvResult::SUCCESS)
      return nullptr;

    unsigned server_to_client_total;
    if (detail::GetMember(obj, "serverToClientTotal",
        &server_to_client_total) != detail::ConvResult::SUCCESS)
      return nullptr;

    unsigned server_to_client_added;
    if (detail::GetMember(obj, "serverToClientAdded",
        &server_to_client_added) != detail::ConvResult::SUCCESS)
      return nullptr;

    unsigned server_to_client_updated;
    if (detail::GetMember(obj, "serverToClientUpdated",
        &server_to_client_updated) != detail::ConvResult::SUCCESS)
      return nullptr;

    unsigned server_to_client_removed;
    if (detail::GetMember(obj, "serverToClientRemoved",
        &server_to_client_removed) != detail::ConvResult::SUCCESS)
      return nullptr;

    unsigned client_to_server_total;
    if (detail::GetMember(obj, "clientToServerTotal",
        &client_to_server_total) != detail::ConvResult::SUCCESS)
      return nullptr;

    unsigned client_to_server_added;
    if (detail::GetMember(obj, "clientToServerAdded",
        &client_to_server_added) != detail::ConvResult::SUCCESS)
      return nullptr;

    unsigned client_to_server_updated;
    if (detail::GetMember(obj, "clientToServerUpdated",
        &client_to_server_updated) != detail::ConvResult::SUCCESS)
      return nullptr;

    unsigned client_to_server_removed;
    if (detail::GetMember(obj, "clientToServerRemoved",
        &client_to_server_removed) != detail::ConvResult::SUCCESS)
      return nullptr;

    result.reset(new SyncStatistics(
        sync_status, sync_service_type, last_sync_time,
        server_to_client_total, server_to_client_added,
        server_to_client_updated, server_to_client_removed,
        client_to_server_total, client_to_server_added,
        client_to_server_updated, client_to_server_removed));

    return result;
  }
};

template <>
struct SerializationDefinition<SyncServiceInfo> {
  static picojson::value ToJson(const SyncServiceInfo& type) {
    picojson::object obj;
    obj["enable"] = picojson::value(type.enable());
    obj["serviceType"] = picojson::value(
        SyncServiceInfo::SyncServiceTypeToString(type.sync_service_type()));
    obj["serverDatabaseUri"] = picojson::value(type.server_database_uri());

    std::string id = type.id();
    if (!id.empty()) {
      obj["id"] = picojson::value(id);
    }

    std::string password = type.password();
    if (!password.empty()) {
      obj["password"] = picojson::value(password);
    }

    return picojson::value(obj);
  }

  static std::unique_ptr<SyncServiceInfo> FromJson(
      const picojson::value& value) {
    if (!value.is<picojson::object>()) {
      return nullptr;
    }
    const picojson::object& obj = value.get<picojson::object>();

    bool enable;
    if (detail::GetMember(obj, "enable",
        &enable) != detail::ConvResult::SUCCESS)
      return nullptr;

    SyncServiceInfo::SyncServiceType serviceType;
    if (detail::GetMember<SyncServiceInfo::SyncServiceType, std::string>(
        obj, "serviceType",
        std::bind(&SyncServiceInfo::ConvertToSyncServiceType,
            std::placeholders::_1),
        &serviceType) != detail::ConvResult::SUCCESS)
      return nullptr;

    std::string serverDatabaseUri;
    if (detail::GetMember(obj, "serverDatabaseUri",
        &serverDatabaseUri) != detail::ConvResult::SUCCESS)
      return nullptr;

    std::string id;
    std::string password;
    detail::GetMember(obj, "id", &id);
    detail::GetMember(obj, "password", &password);

    std::unique_ptr<SyncServiceInfo> result(
        new SyncServiceInfo(enable,
                            serviceType,
                            serverDatabaseUri,
                            id,
                            password));
    return result;
  }
};

template <>
struct SerializationDefinition<SyncInfo> {
  static picojson::value ToJson(const SyncInfo& type) {
    picojson::object obj;
    obj["url"] = picojson::value(type.url());
    obj["id"] = picojson::value(type.id());
    obj["password"] = picojson::value(type.password());

    SyncInfo::SyncMode mode = type.sync_mode();

    obj["mode"] = picojson::value(SyncInfo::SyncModeToString(mode));

    if (mode == SyncInfo::MANUAL_MODE) {
      obj["type"] =
          picojson::value(SyncInfo::SyncTypeToString(type.sync_type()));
    } else if (mode == SyncInfo::PERIODIC_MODE) {
      obj["interval"] = picojson::value(
          SyncInfo::SyncIntervalToString(type.sync_interval()));
    }

    return picojson::value(obj);
  }

  static std::unique_ptr<SyncInfo> FromJson(const picojson::value& value) {
    if (!value.is<picojson::object>()) {
      return nullptr;
    }
    const picojson::object& obj = value.get<picojson::object>();

    std::string url;
    if (detail::GetMember(obj, "url",
        &url) != detail::ConvResult::SUCCESS)
      return nullptr;

    std::string id;
    if (detail::GetMember(obj, "id",
        &id) != detail::ConvResult::SUCCESS)
      return nullptr;

    std::string password;
    if (detail::GetMember(obj, "password",
        &password) != detail::ConvResult::SUCCESS)
      return nullptr;

    SyncInfo::SyncMode mode;
    if (detail::GetMember<SyncInfo::SyncMode, std::string>(
        obj, "mode",
        std::bind(SyncInfo::ConvertToSyncMode, std::placeholders::_1),
        &mode) != detail::ConvResult::SUCCESS)
      return nullptr;

    std::unique_ptr<SyncInfo> result;
    if (mode == SyncInfo::MANUAL_MODE) {
      SyncInfo::SyncType type;
      detail::ConvResult res =
          detail::GetMember<SyncInfo::SyncType, std::string>(
            obj, "type",
            std::bind(SyncInfo::ConvertToSyncType, std::placeholders::_1),
            &type);

      if (res == detail::ConvResult::ERROR) {
        return nullptr;
      } else if (res == detail::ConvResult::SUCCESS) {
        result.reset(new SyncInfo(url, id, password, mode, type,
                                  SyncInfo::INTERVAL_UNDEFINED));
      } else {
        result.reset(new SyncInfo(url, id, password, mode,
                                  SyncInfo::UNDEFINED_TYPE,
                                  SyncInfo::INTERVAL_UNDEFINED));
      }
    } else if (mode == SyncInfo::PERIODIC_MODE) {
      SyncInfo::SyncInterval interval;
      detail::ConvResult res =
          detail::GetMember<SyncInfo::SyncInterval, std::string>(
              obj, "interval",
              std::bind(SyncInfo::ConvertToSyncInterval,
                  std::placeholders::_1),
              &interval);

      if (res == detail::ConvResult::ERROR) {
        return nullptr;
      } else if (res == detail::ConvResult::SUCCESS) {
        result.reset(new SyncInfo(url, id, password, mode,
                                  SyncInfo::UNDEFINED_TYPE, interval));
      } else {
        result.reset(new SyncInfo(url, id, password, mode,
                                  SyncInfo::UNDEFINED_TYPE,
                                  SyncInfo::INTERVAL_UNDEFINED));
      }
    } else {
      result.reset(new SyncInfo(url, id, password, mode,
                                SyncInfo::UNDEFINED_TYPE,
                                SyncInfo::INTERVAL_UNDEFINED));
    }

    return result;
  }
};

template <>
struct SerializationDefinition<SyncProfileInfo> {
  static picojson::value ToJson(const SyncProfileInfo& type) {
    picojson::object obj;
    obj["profileId"] = picojson::value(type.profile_id());
    obj["profileName"] = picojson::value(type.profile_name());
    obj["syncInfo"] =
        SerializationDefinition<SyncInfo>::ToJson(*type.sync_info());

    picojson::array array;
    for (const auto& element : *type.service_info()) {
      array.push_back(
          SerializationDefinition<SyncServiceInfo>::ToJson(*element));
    }

    if (array.empty()) {
      obj["serviceInfo"] = picojson::value();
    } else {
      obj["serviceInfo"] = picojson::value(array);
    }

    return picojson::value(obj);
  }

  static std::unique_ptr<SyncProfileInfo> FromJson(
      const picojson::value& value) {
    std::unique_ptr<SyncProfileInfo> result;

    if (!value.is<picojson::object>()) {
      return nullptr;
    }
    const picojson::object& obj = value.get<picojson::object>();

    std::string profileName;
    if (detail::GetMember(obj, "profileName", &profileName)
        != detail::ConvResult::SUCCESS)
      return nullptr;

    auto syncInfoIt = obj.find("syncInfo");
    if (syncInfoIt == obj.end()) {
      return nullptr;
    }
    std::unique_ptr<SyncInfo> syncInfo =
        SerializationDefinition<SyncInfo>::FromJson(syncInfoIt->second);
    if (!syncInfo) {
      return nullptr;
    }

    SyncServiceInfoListPtr serviceList =
        std::make_shared<SyncServiceInfoList>();

    auto serviceInfoIt = obj.find("serviceInfo");
    if (serviceInfoIt == obj.end()) {
      return nullptr;
    }
    if (serviceInfoIt->second.is<picojson::array>()) {
      const picojson::array& array =
          serviceInfoIt->second.get<picojson::array>();
      for (const auto& element : array) {
        serviceList->push_back(SyncServiceInfoPtr(
            SerializationDefinition<SyncServiceInfo>::FromJson(element)
            .release()));
      }
    } else if (!serviceInfoIt->second.is<picojson::null>()) {
      return nullptr;
    }

    std::string profileId;
    detail::GetMember(obj, "profileId", &profileId);

    result.reset(new SyncProfileInfo(profileId, profileName,
                                     SyncInfoPtr(syncInfo.release()),
                                     serviceList));
    return result;
  }
};

template <class T>
picojson::value ToJson(const T& type) {
  return SerializationDefinition<T>::ToJson(type);
}

template <class T>
std::unique_ptr<T> FromJson(const picojson::value& value) {
  return SerializationDefinition<T>::FromJson(value);
}

}  // namespace serialization
}  // namespace datasync

#endif  // DATASYNC_DATASYNC_SERIALIZATION_H_

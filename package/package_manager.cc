// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "package/package_manager.h"

#include <package-manager.h>
#include <pkgmgr-dbinfo.h>
#include <unistd.h>

#include "common/picojson.h"
#include "package/package_extension_utils.h"
#include "package/package_info.h"
#include "tizen/tizen.h"

namespace {

void PackageRequestsCallback(int id, const char *type, const char *package,
                             package_manager_event_type_e event_type,
                             package_manager_event_state_e event_state,
                             int progress, package_manager_error_e error,
                             void *user_data) {
  std::string pkg_id(package);
  PackageManager* manager = static_cast<PackageManager*> (user_data);
  manager->OnPackageRequestCallback(id, pkg_id, event_type,
                                    event_state, progress);
}

void PackageEventsCallback(const char *type, const char *package,
                           package_manager_event_type_e event_type,
                           package_manager_event_state_e event_state,
                           int progress, package_manager_error_e error,
                           void *user_data) {
  if (event_state != PACKAGE_MANAGER_EVENT_STATE_COMPLETED)
    return;

  std::string pkg_id(package);
  PackageManager* manager = static_cast<PackageManager*> (user_data);
  manager->OnPackageInfoEvent(pkg_id, event_type);
}

}  // namespace

PackageManager::PackageManager(PackageInstance* instace)
    : instance_(instace) {
  int ret = package_manager_create(&manager_);
  if (ret != PACKAGE_MANAGER_ERROR_NONE)
    std::cerr << "package manager creation failed." << std::endl;

  ret = package_manager_request_create(&request_handle_);
  if (ret != PACKAGE_MANAGER_ERROR_NONE) {
    std::cerr << "package manager fail to create request handle." << std::endl;
  } else {
    // Install/Uninstall works only in quiet mode for now.
    package_manager_request_set_mode(request_handle_,
                                     PACKAGE_MANAGER_REQUEST_MODE_QUIET);
    package_manager_request_set_event_cb(request_handle_,
                                         PackageRequestsCallback,
                                         this);
  }
}

PackageManager::~PackageManager() {}

void PackageManager::PackageManagerDestroy() {
  if (request_handle_) {
    package_manager_request_unset_event_cb(request_handle_);
    package_manager_request_destroy(request_handle_);
  }

  if (manager_)
    package_manager_destroy(manager_);
}

picojson::value* PackageManager::InstallPackage(
    const char* pkg_path,
    double callback_id) {
  int request_id;
  int ret = package_manager_request_install(
      request_handle_,
      pkg_path,
      &request_id);
  if (ret != PACKAGE_MANAGER_ERROR_NONE)
    return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);

  callbacks_id_map_[request_id] = callback_id;
  return NULL;
}

picojson::value* PackageManager::UnInstallPackage(
    const char* pkg_id,
    double callback_id) {
  int request_id;
  int ret = package_manager_request_uninstall(request_handle_,
                                              pkg_id,
                                              &request_id);
  if (ret != PACKAGE_MANAGER_ERROR_NONE)
    return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);

  callbacks_id_map_[request_id] = callback_id;
  return NULL;
}

picojson::value* PackageManager::RegisterPackageInfoEvent() {
  int ret = package_manager_set_event_cb(manager_,
                                         PackageEventsCallback,
                                         this);
  if (ret != PACKAGE_MANAGER_ERROR_NONE)
    return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);

  return CreateResultMessage();
}

picojson::value* PackageManager::UnregisterPackageInfoEvent() {
  if (package_manager_unset_event_cb(manager_) != PACKAGE_MANAGER_ERROR_NONE)
    return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);

  return CreateResultMessage();
}

void PackageManager::OnPackageRequestCallback(
    int id,
    const std::string& pkg_id,
    int event_type, int event_state,
    int progress) {
  picojson::object result;
  if (event_state != PACKAGE_MANAGER_EVENT_STATE_FAILED) {
    result["id"] = picojson::value(pkg_id);
    result["cmd"] = picojson::value(static_cast<double>(event_state));
    if (event_state != PACKAGE_MANAGER_EVENT_STATE_PROCESSING)
      result["progress"] = picojson::value(static_cast<double>(progress));
  } else {
    result["error"] =
      picojson::value(static_cast<double>(WebApiAPIErrors::UNKNOWN_ERR));
  }

  double callback_id = 0;
  std::map<int, double>::const_iterator iter = callbacks_id_map_.find(id);
  if (iter != callbacks_id_map_.end())
    callback_id = iter->second;
  instance_->PostPackageRequestMessage(result, callback_id);
}

void PackageManager::OnPackageInfoEvent(
    const std::string& pkg_id,
    int event_type) {
  picojson::object result;
  if (event_type == PACAKGE_MANAGER_EVENT_TYPE_INSTALL ||
      event_type == PACKAGE_MANAGER_EVENT_TYPE_UPDATE) {
    PackageInformation pkg_info(pkg_id, false);
    if (pkg_info.IsValid())
      result["data"] = pkg_info.Value();
  } else if (event_type == PACKAGE_MANAGER_EVENT_TYPE_UNINSTALL) {
    result["data"] = picojson::value(pkg_id);
  }

  result["eventType"] = picojson::value(static_cast<double>(event_type));
  instance_->PostPackageInfoEventMessage(result);
}

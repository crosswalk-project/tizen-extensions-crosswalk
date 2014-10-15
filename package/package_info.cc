// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "package/package_info.h"

#include <package-manager.h>
#include <pkgmgr-dbinfo.h>
#include <pkgmgr-info.h>

#include <unistd.h>
#include <vector>

#include "common/utils.h"
#include "package/package_extension_utils.h"
#include "tizen/tizen.h"

namespace {

void SetErrorMessage(picojson::object& error,
                     const std::string& property_name) {
  std::string error_message = "Fail to get " + property_name;
  if (property_name == "pkginfo" || property_name == "pkgid")
    error["code"] = picojson::value(
        static_cast<double>(WebApiAPIErrors::NOT_FOUND_ERR));
  else
    error["code"] = picojson::value(
        static_cast<double>(WebApiAPIErrors::UNKNOWN_ERR));
  error["message"] = picojson::value(error_message);
  std::cerr << error_message << std::endl;
}

class PkgMgrHandle {
 public:
  static PkgMgrHandle* Create(const std::string& id,
                              bool is_app_id,
                              picojson::object& error) {
    pkgmgr_pkginfo_h pkginfo_handle;
    int ret = PMINFO_R_OK;
    char* pkg_id = NULL;
    uid_t uid = getuid();
    // Get the package ID of the current application.
    if (is_app_id) {
      pkgmgrinfo_appinfo_h handle;
      if (uid != GLOBAL_USER)
        ret = pkgmgrinfo_appinfo_get_usr_appinfo(id.c_str(), uid, &handle);
      else
        ret = pkgmgrinfo_appinfo_get_appinfo(id.c_str(), &handle);

      if (ret != PMINFO_R_OK) {
        SetErrorMessage(error, "pkginfo");
        return NULL;
      }

      ret = pkgmgrinfo_appinfo_get_pkgid(handle, &pkg_id);
      pkgmgrinfo_appinfo_destroy_appinfo(handle);
      if (ret != PMINFO_R_OK) {
        SetErrorMessage(error, "pkginfo");
        return NULL;
      }
    } else {
      pkg_id = const_cast<char*>(id.c_str());
    }

    if (uid != GLOBAL_USER)
      ret = pkgmgr_pkginfo_get_usr_pkginfo(pkg_id, uid, &pkginfo_handle);
    else
      ret = pkgmgr_pkginfo_get_pkginfo(pkg_id, &pkginfo_handle);

    if (ret != PMINFO_R_OK) {
      SetErrorMessage(error, "pkginfo");
      return NULL;
    }

    return new PkgMgrHandle(pkg_id, pkginfo_handle, true);
  }

  static PkgMgrHandle* Create(pkgmgrinfo_pkginfo_h& pkginfo_handle,
                              picojson::object& error) {
    char* pkg_id = NULL;
    int ret = pkgmgrinfo_pkginfo_get_pkgid(pkginfo_handle, &pkg_id);
    if (ret != PMINFO_R_OK || !pkg_id) {
      SetErrorMessage(error, "pkgid");
      return NULL;
    }

    return new PkgMgrHandle(pkg_id, pkginfo_handle, false);
  }

  ~PkgMgrHandle() {
    if (owns_pkginfo_handle_)
      pkgmgrinfo_pkginfo_destroy_pkginfo(pkginfo_handle_);
  }

  const std::string& pkg_id() const { return pkg_id_; }

  bool GetName(char** name, picojson::object& error) {
    int ret = pkgmgrinfo_pkginfo_get_pkgname(pkginfo_handle_, name);
    if (ret != PMINFO_R_OK || *name == NULL) {
      SetErrorMessage(error, "name");
      return false;
    }
    return true;
  }

  bool GetIcon(char** icon_path, picojson::object& error) {
    int ret = pkgmgrinfo_pkginfo_get_icon(pkginfo_handle_, icon_path);
    if ((ret != PMINFO_R_OK) || (*icon_path == NULL)) {
      SetErrorMessage(error, "iconPath");
      return false;
    }
    return true;
  }

  bool GetVersion(char** version, picojson::object& error) {
    int ret = pkgmgrinfo_pkginfo_get_version(pkginfo_handle_, version);
    if ((ret != PMINFO_R_OK) || (*version == NULL)) {
      SetErrorMessage(error, "version");
      return false;
    }
    return true;
  }

  bool GetTotalSize(int* total_size, picojson::object& error) {
    int ret = pkgmgrinfo_pkginfo_get_total_size(pkginfo_handle_, total_size);
    if (ret != PMINFO_R_OK) {
      SetErrorMessage(error, "totalSize");
      return false;
    }
    return true;
  }

  bool GetDataSize(int* data_size, picojson::object& error) {
    int ret = pkgmgrinfo_pkginfo_get_data_size(pkginfo_handle_, data_size);
    if (ret != PMINFO_R_OK) {
      SetErrorMessage(error, "dataSize");
      return false;
    }
    return true;
  }

  bool GetModifiedDate(int* date, picojson::object& error) {
    int ret = pkgmgrinfo_pkginfo_get_installed_time(pkginfo_handle_, date);
    if (ret != PMINFO_R_OK) {
      SetErrorMessage(error, "lastModified");
      return false;
    }
    return true;
  }

  bool GetAuthor(char** author, picojson::object& error) {
    int ret = pkgmgrinfo_pkginfo_get_author_name(pkginfo_handle_, author);
    if (ret != PMINFO_R_OK) {
      SetErrorMessage(error, "author");
      return false;
    }
    return true;
  }

  bool GetDescription(char** description, picojson::object& error) {
    int ret = pkgmgrinfo_pkginfo_get_description(pkginfo_handle_, description);
    if (ret != PMINFO_R_OK) {
      SetErrorMessage(error, "description");
      return false;
    }
    return true;
  }

  bool GetAppIds(std::vector<picojson::value>* appIds,
                 picojson::object& error) {
    int ret = pkgmgrinfo_appinfo_get_list(pkginfo_handle_, PMINFO_ALL_APP,
        &PkgMgrHandle::SaveAppIdsCallback, &appIds);
    if (ret != PMINFO_R_OK) {
      SetErrorMessage(error, "appIds");
      return false;
    }
    return true;
  }

 private:
  static int SaveAppIdsCallback(const pkgmgrinfo_appinfo_h handle, void* data) {
    char* app_id = NULL;
    int ret = pkgmgrinfo_appinfo_get_appid(handle, &app_id);
    if (ret != PMINFO_R_OK)
      return -1;

    std::vector<picojson::value>* appIds =
        static_cast<std::vector<picojson::value>*>(data);
    appIds->push_back(picojson::value(app_id));

    return 1;
  }

  PkgMgrHandle(const std::string& pkg_id,
               pkgmgrinfo_pkginfo_h pkginfo_handle,
               bool owns_pkginfo_handle)
    : pkg_id_(pkg_id),
      pkginfo_handle_(pkginfo_handle),
      owns_pkginfo_handle_(owns_pkginfo_handle) {
  }

  std::string pkg_id_;
  pkgmgrinfo_pkginfo_h pkginfo_handle_;
  bool owns_pkginfo_handle_;

  DISALLOW_COPY_AND_ASSIGN(PkgMgrHandle);
};

void RetrieveAppInfo(PkgMgrHandle& handle,
                     picojson::object& info,
                     picojson::object& error) {
  char* name = NULL;
  char* icon_path = NULL;
  char* version = NULL;
  int total_size = 0;
  int data_size = 0;
  int modified_date = 0;
  char* author = NULL;
  char* description = NULL;
  std::vector<picojson::value> appIds;

  if (!handle.GetName(&name, error) ||
      !handle.GetIcon(&icon_path, error) ||
      !handle.GetVersion(&version, error) ||
      !handle.GetTotalSize(&total_size, error) ||
      !handle.GetDataSize(&data_size, error) ||
      !handle.GetModifiedDate(&modified_date, error) ||
      !handle.GetAuthor(&author, error) ||
      !handle.GetDescription(&description, error) ||
      !handle.GetAppIds(&appIds, error)) {
    return;
  }

  info["id"] = picojson::value(handle.pkg_id());
  info["name"] = picojson::value(name);
  info["iconPath"] = picojson::value(icon_path);
  info["version"] = picojson::value(version);
  info["totalSize"] = picojson::value(static_cast<double>(total_size));
  info["dataSize"] = picojson::value(static_cast<double>(data_size));
  info["lastModified"] = picojson::value(static_cast<double>(modified_date));
  info["author"] = picojson::value(author);
  info["description"] = picojson::value(description);
  info["appIds"] = picojson::value(appIds);
}

int GetPackagesInfoCallback(pkgmgrinfo_pkginfo_h pkginfo_handle,
                            void* user_data) {
  picojson::array* data = static_cast<picojson::array*>(user_data);
  picojson::object info;
  picojson::object error;

  std::unique_ptr<PkgMgrHandle> handle(
      PkgMgrHandle::Create(pkginfo_handle, error));
  if (!handle) {
    data->clear();
    return -1;
  }

  RetrieveAppInfo(*handle, info, error);
  if (!error.empty()) {
    data->clear();
    return -1;
  }

  data->push_back(picojson::value(info));
  return 0;
}

void RetrieveAllInstalledPackagesInfo(picojson::array& data,
                                      picojson::object& error) {
  int ret = PMINFO_R_OK;
  uid_t uid = getuid();

  if (uid != GLOBAL_USER)
    ret = pkgmgrinfo_pkginfo_get_usr_list(GetPackagesInfoCallback, &data, uid);
  else
    ret = pkgmgrinfo_pkginfo_get_list(GetPackagesInfoCallback, &data);

  if (ret != PMINFO_R_OK) {
    SetErrorMessage(error, "get_all");
    return;
  }

  if (data.empty())
    SetErrorMessage(error, "get_all");
}

}  // namespace

picojson::value* PackageInformation::GetAllInstalled() {
  picojson::array data;
  picojson::object error;

  RetrieveAllInstalledPackagesInfo(data, error);
  if (!error.empty())
    return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);
  else
    return CreateResultMessage(data);
}

PackageInformation::PackageInformation(
    const std::string& id,
    bool is_app_id) {
  std::unique_ptr<PkgMgrHandle> handle(PkgMgrHandle::Create(
      id, is_app_id, error_));
  if (handle && IsValid())
    RetrieveAppInfo(*handle, data_, error_);
}

PackageInformation::~PackageInformation() {}

const picojson::value& PackageInformation::Value() {
  if (value_.is<picojson::null>() && IsValid())
    value_ = picojson::value(data_);

  return value_;
}

const std::string PackageInformation::Serialize() {
  std::unique_ptr<picojson::value> result;
  if (!IsValid()) {
    picojson::object::const_iterator it = error_.find("code");
    assert(it != error_.end());
    result.reset(CreateResultMessage(
        static_cast<WebApiAPIErrors>(it->second.get<double>())));
  } else {
    result.reset(CreateResultMessage(Value()));
  }

  return result->serialize();
}

bool PackageInformation::IsValid() const {
  return error_.empty();
}

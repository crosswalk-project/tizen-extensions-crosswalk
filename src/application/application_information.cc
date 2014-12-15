// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "application/application_information.h"

#include <package_info.h>
#include <package_manager.h>
#include <package-manager.h>
#include <pkgmgr-info.h>
#include <tzplatform_config.h>
#include <unistd.h>

#include <memory>
#include <utility>
#include <vector>

#include "application/application_extension_utils.h"
#include "tizen/tizen.h"

namespace {

const uid_t GLOBAL_USER = tzplatform_getuid(TZ_SYS_GLOBALAPP_USER);

void SetErrorMessage(picojson::object& error,
                     const std::string& property_name) {
  std::string error_message = "Fail to get " + property_name;
  if (property_name == "appinfo" || property_name == "appid")
    error["code"] = picojson::value(
        static_cast<double>(WebApiAPIErrors::NOT_FOUND_ERR));
  else
    error["code"] = picojson::value(
        static_cast<double>(WebApiAPIErrors::UNKNOWN_ERR));
  error["message"] = picojson::value(error_message);
  std::cerr << error_message << std::endl;
}

// This class contains both appinfo and pkginfo handles for an application, it
// will manage both handles' lifetime. A special case is when it is instantiated
// by an existing appinfo handle(e.g. in pkgmgrinfo_app_list_cb callback), the
// appinfo handle will not owned by it.
class PkgMgrHandle {
 public:
  static PkgMgrHandle* Create(const std::string& app_id,
                              picojson::object& error) {
    pkgmgrinfo_appinfo_h appinfo_handle;
    uid_t uid = getuid();
    int ret = (uid != GLOBAL_USER) ?
              pkgmgrinfo_appinfo_get_usr_appinfo(app_id.c_str(),
                                                 uid, &appinfo_handle) :
              pkgmgrinfo_appinfo_get_appinfo(app_id.c_str(),
                                             &appinfo_handle);
    if (ret != PMINFO_R_OK) {
      SetErrorMessage(error, "appinfo");
      return NULL;
    }
    return CreateInternal(app_id, appinfo_handle, true, error);
  }

  static PkgMgrHandle* Create(pkgmgrinfo_appinfo_h& appinfo_handle,
                              picojson::object& error) {
    char* app_id = NULL;
    int ret = pkgmgrinfo_appinfo_get_appid(appinfo_handle, &app_id);
    if (ret != PMINFO_R_OK || !app_id) {
      SetErrorMessage(error, "appid");
      return NULL;
    }
    return CreateInternal(app_id, appinfo_handle, false, error);
  }

  ~PkgMgrHandle() {
    if (owns_appinfo_handle_)
      pkgmgrinfo_appinfo_destroy_appinfo(appinfo_handle_);
    pkgmgrinfo_pkginfo_destroy_pkginfo(pkginfo_handle_);
  }

  const std::string& app_id() const { return app_id_; }

  const std::string& pkg_id() const { return pkg_id_; }

  bool GetName(char** name, picojson::object& error) {
    int ret = pkgmgrinfo_appinfo_get_label(appinfo_handle_, name);
    if ((ret != PMINFO_R_OK) || (*name == NULL)) {
      SetErrorMessage(error, "name");
      return false;
    }
    return true;
  }

  bool GetIcon(char** icon_path, picojson::object& error) {
    int ret = pkgmgrinfo_appinfo_get_icon(appinfo_handle_, icon_path);
    if ((ret != PMINFO_R_OK) || (*icon_path == NULL)) {
      SetErrorMessage(error, "icon_path");
      return false;
    }
    return true;
  }

  bool GetNoDisplay(bool* show, picojson::object& error) {
    int ret = pkgmgrinfo_appinfo_is_nodisplay(appinfo_handle_, show);
    if (ret != PMINFO_R_OK) {
      SetErrorMessage(error, "show");
      return false;
    }

    *show = !(*show);
    return true;
  }

  bool GetCategories(std::vector<picojson::value>* categories,
                     picojson::object& error) {
    int ret = pkgmgrinfo_appinfo_foreach_category(
        appinfo_handle_, &PkgMgrHandle::SaveCategoriesCallback, &categories);
    if (ret != PMINFO_R_OK) {
      SetErrorMessage(error, "categories");
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

  bool GetInstallTime(int* install_date, picojson::object& error) {
    int ret = pkgmgrinfo_pkginfo_get_installed_time(
        pkginfo_handle_, install_date);
    if (ret != PMINFO_R_OK) {
      SetErrorMessage(error, "install_time");
      return false;
    }
    return true;
  }

  bool GetSize(int* install_size, picojson::object& error) {
    int ret = pkgmgrinfo_pkginfo_get_total_size(pkginfo_handle_, install_size);
    if (ret != PMINFO_R_OK) {
      SetErrorMessage(error, "install_size");
      return false;
    }
    return true;
  }

 private:
  static PkgMgrHandle* CreateInternal(const std::string& app_id,
                                      pkgmgrinfo_appinfo_h& appinfo_handle,
                                      bool owns_appinfo_handle,
                                      picojson::object& error) {
    char* pkg_id = NULL;
    int ret = package_manager_get_package_id_by_app_id(app_id.c_str(), &pkg_id);
    if (ret != PACKAGE_MANAGER_ERROR_NONE) {
      SetErrorMessage(error, "pkgid");
      pkgmgrinfo_appinfo_destroy_appinfo(appinfo_handle);
      return NULL;
    }

    pkgmgrinfo_pkginfo_h pkginfo_handle;
    uid_t uid = getuid();
    ret = (uid != GLOBAL_USER) ?
           pkgmgrinfo_pkginfo_get_usr_pkginfo(pkg_id, uid, &pkginfo_handle) :
           pkgmgrinfo_pkginfo_get_pkginfo(pkg_id, &pkginfo_handle);
    if (ret != PMINFO_R_OK) {
      SetErrorMessage(error, "pkginfo");
      pkgmgrinfo_appinfo_destroy_appinfo(appinfo_handle);
      return NULL;
    }
    return new PkgMgrHandle(app_id, pkg_id, appinfo_handle,
                            pkginfo_handle, owns_appinfo_handle);
  }

  static int SaveCategoriesCallback(const char* category, void* user_data) {
    if (category) {
      std::vector<picojson::value>* categories =
          static_cast<std::vector<picojson::value>*>(user_data);
      categories->push_back(picojson::value(category));
    }
    return 1;
  }

  PkgMgrHandle(const std::string& app_id,
               const std::string& pkg_id,
               pkgmgrinfo_appinfo_h appinfo_handle,
               pkgmgrinfo_pkginfo_h pkginfo_handle,
               bool owns_appinfo_handle)
    : app_id_(app_id),
      pkg_id_(pkg_id),
      appinfo_handle_(appinfo_handle),
      pkginfo_handle_(pkginfo_handle),
      owns_appinfo_handle_(owns_appinfo_handle) {
  }

  std::string app_id_;
  std::string pkg_id_;
  pkgmgrinfo_appinfo_h appinfo_handle_;
  pkgmgrinfo_pkginfo_h pkginfo_handle_;
  bool owns_appinfo_handle_;
};

void RetrieveAppInfo(PkgMgrHandle& handle,
                     picojson::object& info,
                     picojson::object& error) {
  char* name = NULL;
  char* icon_path = NULL;
  char* version = NULL;
  int install_date = 0;
  int install_size = 0;
  bool show = true;
  std::vector<picojson::value> categories;

  if (!handle.GetName(&name, error) ||
      !handle.GetIcon(&icon_path, error) ||
      !handle.GetNoDisplay(&show, error) ||
      !handle.GetCategories(&categories, error) ||
      !handle.GetVersion(&version, error) ||
      !handle.GetInstallTime(&install_date, error) ||
      !handle.GetSize(&install_size, error)) {
    return;
  }

  info["id"] = picojson::value(handle.app_id());
  info["name"] = picojson::value(name);
  info["iconPath"] = picojson::value(icon_path);
  info["version"] = picojson::value(version);
  info["show"] = picojson::value(show);
  info["categories"] = picojson::value(categories);
  info["installDate"] = picojson::value(static_cast<double>(install_date));
  info["size"] = picojson::value(static_cast<double>(install_size));
  info["packageId"] = picojson::value(handle.pkg_id());
}

int GetAllAppInfoCallback(pkgmgrinfo_appinfo_h appinfo_handle,
                          void* user_data) {
  picojson::array* data = static_cast<picojson::array*>(user_data);
  picojson::object info;
  picojson::object error;

  std::unique_ptr<PkgMgrHandle> handle(
      PkgMgrHandle::Create(appinfo_handle, error));
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

void RetrieveAllInstalledAppInfo(picojson::array& data,
                                 picojson::object& error) {
  uid_t uid = getuid();
  int ret = (uid != GLOBAL_USER) ?
             pkgmgrinfo_appinfo_get_usr_installed_list(GetAllAppInfoCallback,
                                                       uid, &data) :
             pkgmgrinfo_appinfo_get_installed_list(GetAllAppInfoCallback,
                                                   &data);
  if (ret != PMINFO_R_OK) {
    SetErrorMessage(error, "installed");
    return;
  }

  if (data.empty())
    SetErrorMessage(error, "get_all");
}

}  // namespace

picojson::value* ApplicationInformation::GetAllInstalled() {
  picojson::array data;
  picojson::object error;

  RetrieveAllInstalledAppInfo(data, error);
  if (!error.empty())
    return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);
  else
    return CreateResultMessage(data);
}

ApplicationInformation::ApplicationInformation(const std::string& app_id) {
  std::unique_ptr<PkgMgrHandle> handle(PkgMgrHandle::Create(app_id, error_));
  if (handle && IsValid())
    RetrieveAppInfo(*handle, data_, error_);
}

ApplicationInformation::~ApplicationInformation() {
}

const picojson::value& ApplicationInformation::Value() {
  if (value_.is<picojson::null>() && IsValid())
    value_ = picojson::value(data_);
  return value_;
}

const std::string ApplicationInformation::Serialize() {
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

bool ApplicationInformation::IsValid() const {
  return error_.empty();
}

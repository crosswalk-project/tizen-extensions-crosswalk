// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "application/application_information.h"

#include <package_info.h>
#include <package_manager.h>
#include <package-manager.h>
#include <pkgmgr-info.h>

#include <memory>
#include <vector>

namespace {

void SetErrorMessage(picojson::value& error, const std::string& property_name) {
  std::string error_message = "Fail to get " + property_name;
  error.get<picojson::object>()["error"] = picojson::value(property_name);
  error.get<picojson::object>()["message"] = picojson::value(error_message);
  std::cerr << error_message << std::endl;
}

// This class contains both appinfo and pkginfo handles for an application, it
// will manage both handles' lifetime. A special case is when it is instantiated
// by an existing appinfo handle(e.g. in pkgmgrinfo_app_list_cb callback), the
// appinfo handle will not owned by it.
class PkgMgrHandle {
 public:
  static PkgMgrHandle* Create(const std::string& app_id,
                              picojson::value& error) {
    pkgmgrinfo_appinfo_h appinfo_handle;
    int ret = pkgmgrinfo_appinfo_get_appinfo(app_id.c_str(), &appinfo_handle);
    if (ret != PMINFO_R_OK) {
      SetErrorMessage(error, "appinfo");
      return NULL;
    }
    return CreateInternal(app_id, appinfo_handle, true, error);
  }

  static PkgMgrHandle* Create(pkgmgrinfo_appinfo_h& appinfo_handle,
                              picojson::value& error) {
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

  bool GetName(char** name, picojson::value& error) {
    int ret = pkgmgrinfo_appinfo_get_label(appinfo_handle_, name);
    if ((ret != PMINFO_R_OK) || (*name == NULL)) {
      SetErrorMessage(error, "name");
      return false;
    }
    return true;
  }

  bool GetIcon(char** icon_path, picojson::value& error) {
    int ret = pkgmgrinfo_appinfo_get_icon(appinfo_handle_, icon_path);
    if ((ret != PMINFO_R_OK) || (*icon_path == NULL)) {
      SetErrorMessage(error, "icon_path");
      return false;
    }
    return true;
  }

  bool GetNoDisplay(bool* show, picojson::value& error) {
    int ret = pkgmgrinfo_appinfo_is_nodisplay(appinfo_handle_, show);
    if (ret != PMINFO_R_OK) {
      SetErrorMessage(error, "show");
      return false;
    }

    *show = !(*show);
    return true;
  }

  bool GetCategories(std::vector<picojson::value>* categories,
                     picojson::value& error) {
    int ret = pkgmgrinfo_appinfo_foreach_category(
        appinfo_handle_, &PkgMgrHandle::SaveCategoriesCallback, &categories);
    if (ret != PMINFO_R_OK) {
      SetErrorMessage(error, "categories");
      return false;
    }
    return true;
  }

  bool GetVersion(char** version, picojson::value& error) {
    int ret = pkgmgrinfo_pkginfo_get_version(pkginfo_handle_, version);
    if ((ret != PMINFO_R_OK) || (*version == NULL)) {
      SetErrorMessage(error, "version");
      return false;
    }
    return true;
  }

  bool GetInstallTime(int* install_date, picojson::value& error) {
    int ret = pkgmgrinfo_pkginfo_get_installed_time(
        pkginfo_handle_, install_date);
    if (ret != PMINFO_R_OK) {
      SetErrorMessage(error, "install_time");
      return false;
    }
    return true;
  }

  bool GetSize(int* install_size, picojson::value& error) {
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
                                      picojson::value& error) {
    char* pkg_id = NULL;
    int ret = package_manager_get_package_id_by_app_id(app_id.c_str(), &pkg_id);
    if (ret != PACKAGE_MANAGER_ERROR_NONE) {
      SetErrorMessage(error, "pkgid");
      pkgmgrinfo_appinfo_destroy_appinfo(appinfo_handle);
      return NULL;
    }

    pkgmgrinfo_pkginfo_h pkginfo_handle;
    ret = pkgmgrinfo_pkginfo_get_pkginfo(pkg_id, &pkginfo_handle);
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
                     picojson::value& info,
                     picojson::value& error) {
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

  picojson::value::object& data = info.get<picojson::object>();
  data["id"] = picojson::value(handle.app_id());
  data["name"] = picojson::value(name);
  data["iconPath"] = picojson::value(icon_path);
  data["version"] = picojson::value(version);
  data["show"] = picojson::value(show);
  data["categories"] = picojson::value(categories);
  data["installDate"] = picojson::value(static_cast<double>(install_date));
  data["size"] = picojson::value(static_cast<double>(install_size));
  data["packageId"] = picojson::value(handle.pkg_id());
}

int GetAllAppInfoCallback(pkgmgrinfo_appinfo_h appinfo_handle,
                          void* user_data) {
  picojson::array* data = static_cast<picojson::array*>(user_data);
  picojson::value info(picojson::object_type, true);
  picojson::value error(picojson::object_type, true);

  std::unique_ptr<PkgMgrHandle> handle(
      PkgMgrHandle::Create(appinfo_handle, error));
  if (!handle) {
    data->clear();
    return -1;
  }

  RetrieveAppInfo(*handle, info, error);
  if (!error.get<picojson::object>().empty()) {
    data->clear();
    return -1;
  }

  data->push_back(info);
  return 0;
}

void RetrieveAllInstalledAppInfo(picojson::value& data,
                                 picojson::value& error) {
  int ret = pkgmgrinfo_appinfo_get_installed_list(
      GetAllAppInfoCallback, &data.get<picojson::array>());
  if (ret != PMINFO_R_OK) {
    SetErrorMessage(error, "installed");
    return;
  }

  if (data.get<picojson::array>().empty())
    SetErrorMessage(error, "get_all");
}

int PkgIdToAppIdCallback(const pkgmgr_appinfo_h handle, void* user_data) {
  char* app_id = NULL;
  if (pkgmgr_appinfo_get_appid(handle, &app_id) != PMINFO_R_OK)
    return 0;

  std::string* data = static_cast<std::string*>(user_data);
  (*data) = app_id;
  return 0;
}

}  // namespace

std::string ApplicationInformation::PkgIdToAppId(const std::string& pkg_id) {
  pkgmgrinfo_pkginfo_h pkginfo_handle;
  int ret = pkgmgrinfo_pkginfo_get_pkginfo(pkg_id.c_str(), &pkginfo_handle);
  if (ret != PMINFO_R_OK)
    return "";

  std::string app_id;
  // By now, we only have UI Crosswalk application.
  pkgmgr_appinfo_get_list(
      pkginfo_handle, PM_UI_APP, &PkgIdToAppIdCallback, &app_id);
  return app_id;
}

picojson::value* ApplicationInformation::GetAllInstalled() {
  picojson::value* result = new picojson::value(picojson::object_type, true);
  picojson::value data(picojson::array_type, true);
  picojson::value error(picojson::object_type, true);

  RetrieveAllInstalledAppInfo(data, error);
  if (!error.get<picojson::object>().empty())
    result->get<picojson::object>()["error"] = error;
  else
    result->get<picojson::object>()["data"] = data;
  return result;
}

ApplicationInformation::ApplicationInformation(const std::string& app_id)
    : data_(picojson::object_type, true),
      error_(picojson::object_type, true) {
  std::unique_ptr<PkgMgrHandle> handle(PkgMgrHandle::Create(app_id, error_));
  if (handle && IsValid())
    RetrieveAppInfo(*handle, data_, error_);
}

ApplicationInformation::~ApplicationInformation() {
}

std::string ApplicationInformation::Serialize() const {
  if (IsValid()) {
    return data_.serialize();
  } else if (!error_.get<picojson::object>().empty()) {
    return error_.serialize();
  } else {
    return "";
  }
}

bool ApplicationInformation::IsValid() const {
  return error_.get<picojson::object>().empty();
}

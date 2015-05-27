// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "application/application_manager.h"

#include <app_manager.h>
#include <aul.h>
#include <bundle.h>
#include <pkgmgr-info.h>
#include <package_manager.h>
#include <tzplatform_config.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>
// TODO (halton): Remove NOLINT after cpplint.py updated with CL
// https://codereview.chromium.org/269013009/
#include <tuple>  // NOLINT

#include "application/application.h"
#include "application/application_extension_utils.h"
#include "application/application_information.h"
#include "application/application_instance.h"
#include "application/application_requested_control.h"
#include "tizen/tizen.h"

namespace {

const uid_t GLOBAL_USER = tzplatform_getuid(TZ_SYS_GLOBALAPP_USER);

// Application information events.
const char kOkayEvent[] = "ok";
const char kInstallEvent[] = "install";
const char kUpdateEvent[] = "update";
const char kUninstallEvent[] = "uninstall";
const char kEventStart[] = "start";
const char kEventEnd[] = "end";

bool PackageToAppCallback(package_info_app_component_type_e comp_type,
                          const char* app_id, void* user_data) {
  if (app_id == NULL) {
    std::cerr << "No app id passed in the pakage to app callback.\n";
    return true;
  }

  std::vector<std::string>* app_ids =
      static_cast<std::vector<std::string>*>(user_data);
  app_ids->push_back(app_id);
  return true;
}

void RetrievePackageAppIds(const char* package,
                           std::vector<std::string>& app_ids) {
  package_info_h package_info;
  if (package_manager_get_package_info(package, &package_info)
      != PACKAGE_MANAGER_ERROR_NONE) {
    std::cerr << "Cannot create package info.\n";
    return;
  }

  if (package_info_foreach_app_from_package(
        package_info, PACKAGE_INFO_ALLAPP, PackageToAppCallback, &app_ids)
      != PACKAGE_MANAGER_ERROR_NONE) {
    std::cerr << "Failed while getting appids.\n";
    return;
  }

  if (package_info_destroy(package_info) != PACKAGE_MANAGER_ERROR_NONE)
    std::cerr << "Cannot destroy package info.\n";
}

int AppEventsCallback(int id, const char* type, const char* package,
                      const char* key, const char* val,
                      const void* msg, void* data) {
  std::string event_type(val);
  std::string event_state(key);
  std::transform(
      event_type.begin(), event_type.end(), event_type.begin(), ::tolower);
  std::transform(
      event_state.begin(), event_state.end(), event_state.begin(), ::tolower);

  ApplicationManager* manager = static_cast<ApplicationManager*>(data);
  std::vector<std::string> app_ids;
  if ((event_type == kUninstallEvent && event_state == kEventStart) ||
      (event_type != kUninstallEvent && event_state == kEventEnd))
    RetrievePackageAppIds(package, app_ids);
  manager->OnAppInfoEvent(event_type, event_state, app_ids);

  return APP_MANAGER_ERROR_NONE;
}

int AppMetaDataCallback(const char* key, const char* value, void* data) {
  auto ret = static_cast<std::tuple<bool, picojson::array>*>(data);
  if (!key || !value) {
    std::cerr << "Application metadata contains null key/value.\n";
    std::get<0>(*ret) = false;
    return -1;
  }

  picojson::object obj;
  obj["key"] = picojson::value(key);
  obj["value"] = picojson::value(value);
  std::get<1>(*ret).push_back(picojson::value(obj));
  return 0;
}

bool AddAppControlToService(const ApplicationControl& app_control,
    service_h service) {
  if (service_set_operation(service, app_control.operation().c_str())
      != SERVICE_ERROR_NONE) {
    return false;
  }
  if (!app_control.mime().empty()) {
    if (service_set_mime(service, app_control.mime().c_str())
      != SERVICE_ERROR_NONE) {
      return false;
    }
  }
  if (!app_control.uri().empty()) {
    if (service_set_uri(service, app_control.uri().c_str())
        != SERVICE_ERROR_NONE) {
      return false;
    }
  }
  if (!app_control.category().empty()) {
    if (service_set_category(service, app_control.category().c_str())
        != SERVICE_ERROR_NONE) {
      return false;
    }
  }
  if (!AddAppControlDataArrayToService(app_control.app_control_data_array(),
     service))
    return false;
  return true;
}

}  // namespace

ApplicationManager::ApplicationManager()
    : pkgmgr_handle_(NULL) {
}

ApplicationManager::~ApplicationManager() {
  if (pkgmgr_handle_)
    pkgmgr_client_free(pkgmgr_handle_);
}

// FIXME(xiang): Using PID as app context ID may introduce a corner case bug.
// See details at: https://crosswalk-project.org/jira/browse/XWALK-1563.
picojson::value* ApplicationManager::KillApp(const std::string& context_id) {
  pid_t pid = std::stoi(context_id);
  if (pid == getpid())
    return CreateResultMessage(WebApiAPIErrors::INVALID_VALUES_ERR);

  char* app_id = NULL;
  app_context_h app_context = NULL;
  WebApiAPIErrors error = WebApiAPIErrors::NO_ERROR;

  int ret = app_manager_get_app_id(pid, &app_id);
  if (ret != APP_MANAGER_ERROR_NONE) {
    std::cerr << "Fail to get app id by pid: " << ret << std::endl;
    switch (ret) {
      case APP_MANAGER_ERROR_NO_SUCH_APP:
      case APP_MANAGER_ERROR_INVALID_PARAMETER:
        error = WebApiAPIErrors::NOT_FOUND_ERR;
        break;
      default:
        error = WebApiAPIErrors::UNKNOWN_ERR;
    }
    goto end;
  }

  ret = app_manager_get_app_context(app_id, &app_context);
  if (ret != APP_MANAGER_ERROR_NONE) {
    std::cerr << "Fail to get app_context: " << ret << std::endl;
    error = WebApiAPIErrors::NOT_FOUND_ERR;
    goto end;
  }

  ret = app_manager_terminate_app(app_context);
  if (ret != APP_MANAGER_ERROR_NONE) {
    std::cerr << "Fail to termniate app: " << ret << std::endl;
    error = WebApiAPIErrors::UNKNOWN_ERR;
    goto end;
  }

 end:
  if (app_id)
    free(app_id);
  if (app_context)
    app_context_destroy(app_context);
  return error == WebApiAPIErrors::NO_ERROR ?
      CreateResultMessage() : CreateResultMessage(error);
}

picojson::value* ApplicationManager::LaunchApp(const std::string& app_id) {
  int ret = aul_open_app(app_id.c_str());
  if (ret < 0) {
    switch (ret) {
      case AUL_R_EINVAL:
      case AUL_R_ERROR:
        return CreateResultMessage(WebApiAPIErrors::NOT_FOUND_ERR);
      default:
        return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);
    }
  }

  return CreateResultMessage();
}

picojson::value* ApplicationManager::LaunchAppControl(
    const ApplicationControl& app_control, const std::string& app_id,
    int reply_callback_id,
    ReplyCallback post_callback) {
  service_h service;
  if (service_create(&service) != SERVICE_ERROR_NONE) {
    return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);
  }

  if (!AddAppControlToService(app_control, service)) {
    return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);
  }
  if (!app_id.empty()) {
    service_set_app_id(service, app_id.c_str());
  }

  std::unique_ptr<CallbackData> callback_data(
      new CallbackData(reply_callback_id, post_callback, this));
  reply_callbacks_.insert(std::make_pair(reply_callback_id,
      std::move(callback_data)));

  if (service_send_launch_request(service,
      [](service_h request, service_h reply, service_result_e result,
          void* user_data) {
        int reply_id = 0;
        ReplyCallback callback;
        ApplicationManager* that = nullptr;

        std::tie(reply_id, callback, that) =
            *static_cast<CallbackData*>(user_data);

        // TODO(t.iwanek): validate that...

        that->reply_callbacks_.erase(that->reply_callbacks_.find(reply_id));
        picojson::object ret;
        if (result == SERVICE_RESULT_SUCCEEDED) {
          std::unique_ptr<ApplicationControl> app_control =
              ApplicationControl::ApplicationControlFromService(reply);
          picojson::array array;
          for (const auto& item : app_control->app_control_data_array())
            array.push_back(*item->ToJson());
          ret["data"] = picojson::value(array);
        } else {
          ret["error"] = picojson::value(static_cast<double>(
              WebApiAPIErrors::FAILURE_RESPONSE_ERR));
        }

        picojson::value value(ret);
        callback(value, reply_id);
      }, reply_callbacks_[reply_callback_id].get()) != SERVICE_ERROR_NONE) {
    (void) service_destroy(service);
    return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);
  }
  (void) service_destroy(service);
  return new picojson::value(picojson::object());
}

picojson::value* ApplicationManager::FindAppControl(
    const ApplicationControl& app_control) {
  picojson::object result;
  picojson::array output_array;
  service_h service;
  service_create(&service);

  if (!AddAppControlToService(app_control, service))
    return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);

  std::vector<ApplicationInformation> appInfos;
  if (service_foreach_app_matched(service,
        [](service_h service, const char* appid, void* user_data) {
          std::vector<ApplicationInformation>* infos =
              static_cast<std::vector<ApplicationInformation>*>(user_data);
          Application application(appid);
          infos->push_back(application.GetAppInfo());
          return true;
      }, &appInfos) != SERVICE_ERROR_NONE)
    return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);
  service_destroy(service);

  for (auto& info : appInfos) {
    output_array.push_back(info.Value());
  }

  result["data"] = picojson::value(output_array);
  return new picojson::value(result);
}

picojson::value* ApplicationManager::GetAppMetaData(const std::string& app_id) {
  pkgmgrinfo_appinfo_h handle;
  uid_t uid = getuid();
  int ret = (uid != GLOBAL_USER) ?
             pkgmgrinfo_appinfo_get_usr_appinfo(app_id.c_str(),
                                                uid, &handle) :
             pkgmgrinfo_appinfo_get_appinfo(app_id.c_str(),
                                            &handle);
  if (ret != PMINFO_R_OK)
    return CreateResultMessage(WebApiAPIErrors::NOT_FOUND_ERR);
  // The first boolean will set to false if AppMetaDataCallback fail.
  auto data = std::make_tuple(true, picojson::array());
  ret = pkgmgrinfo_appinfo_foreach_metadata(handle, AppMetaDataCallback, &data);
  pkgmgrinfo_appinfo_destroy_appinfo(handle);
  if (ret != PMINFO_R_OK || !std::get<0>(data))
    return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);

  return CreateResultMessage(std::get<1>(data));
}

picojson::value* ApplicationManager::GetRequestedAppControl(
    const std::string& encoded_bundle) {
  picojson::object result;
  auto requested =
      ApplicationRequestedControl::GetRequestedApplicationControl(
          encoded_bundle);
  if (!requested)
    return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);
  auto json = requested->ToJson();
  if (!json)
    return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);
  result["data"] = *json;
  return new picojson::value(result);
}

picojson::value* ApplicationManager::ReplyResult(
    const std::vector<std::unique_ptr<ApplicationControlData>>& data,
    const std::string& encoded_bundle) {
  std::unique_ptr<ApplicationRequestedControl> requested =
      ApplicationRequestedControl::GetRequestedApplicationControl(
          encoded_bundle);
  if (!requested)
    return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);
  if (!requested->app_control()->ReplyResult(data, encoded_bundle))
    return CreateResultMessage(WebApiAPIErrors::NOT_FOUND_ERR);
  return CreateResultMessage();
}

picojson::value* ApplicationManager::ReplyFailure(
    const std::string& encoded_bundle) {
  std::unique_ptr<ApplicationRequestedControl> requested =
      ApplicationRequestedControl::GetRequestedApplicationControl(
          encoded_bundle);
  if (!requested)
    return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);
  if (!requested->app_control()->ReplyFailure(encoded_bundle))
    return CreateResultMessage(WebApiAPIErrors::NOT_FOUND_ERR);
  return CreateResultMessage();
}

picojson::value* ApplicationManager::RegisterAppInfoEvent(
      ApplicationInstance* instance,
      const AppInfoEventCallback& event_callback) {
  if (instances_.empty()) {
    assert(!pkgmgr_handle_);
    pkgmgr_handle_ = pkgmgr_client_new(PC_LISTENING);
    if (!pkgmgr_handle_)
      return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);
    else
      pkgmgr_client_listen_status(pkgmgr_handle_, AppEventsCallback, this);
  }

  if (IsInstanceRegistered(instance)) {
    std::cerr << "Instance already registered app info events.\n";
    return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);
  }

  instances_.insert(std::make_pair(instance, event_callback));
  return CreateResultMessage();
}

picojson::value* ApplicationManager::UnregisterAppInfoEvent(
    ApplicationInstance* instance) {
  auto iter = instances_.find(instance);
  if (iter == instances_.end()) {
    std::cerr << "Instance not registered app info events.\n";
    return CreateResultMessage(WebApiAPIErrors::NOT_FOUND_ERR);
  }

  instances_.erase(iter);
  if (instances_.empty()) {
    assert(pkgmgr_handle_);
    pkgmgr_client_free(pkgmgr_handle_);
    pkgmgr_handle_ = NULL;
  }

  return CreateResultMessage();
}

bool ApplicationManager::IsInstanceRegistered(
    ApplicationInstance* const instance) const {
  return instances_.find(instance) != instances_.end();
}

void ApplicationManager::OnAppInfoEvent(
    const std::string& event_type,
    const std::string& event_state,
    const std::vector<std::string>& app_ids) {
  // Previously started event is finished, then we can send corresponding info
  // to registered instances.
  if (!started_event_.empty() && event_state == kEventEnd &&
      event_type == kOkayEvent) {
    picojson::array data;
    picojson::object result;

    if (started_event_ == kInstallEvent || started_event_ == kUpdateEvent) {
      for (auto it = app_ids.begin(); it != app_ids.end(); ++it) {
        ApplicationInformation app_info(*it);
        if (app_info.IsValid())
          data.push_back(app_info.Value());
      }
      if (started_event_ == kInstallEvent)
        result["installed"] = picojson::value(data);
      else
        result["updated"] = picojson::value(data);
    }

    if (started_event_ == kUninstallEvent) {
      auto it = uninstalled_app_ids_.begin();
      for (; it != uninstalled_app_ids_.end(); ++it)
        data.push_back(picojson::value(*it));
      result["uninstalled"] = picojson::value(data);
      uninstalled_app_ids_.clear();
    }

    started_event_.clear();
    auto instance_it = instances_.begin();
    for (; instance_it != instances_.end(); ++instance_it)
      instance_it->second(result);
  }

  // Save just one event as the events will not notified interleaved.
  if (event_state == kEventStart) {
    if (!started_event_.empty())
      std::cerr << "Event received before previous event finished.\n";
    started_event_ = event_type;
    // After uninstallation we cannot get app ids from package name.
    if (event_type == kUninstallEvent)
      uninstalled_app_ids_ = app_ids;
  }
}

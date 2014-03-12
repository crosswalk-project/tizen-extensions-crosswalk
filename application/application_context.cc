// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "application/application_context.h"

#include <app_manager.h>
#include <memory>
#include <tuple>

#include "application/application_manager.h"

picojson::object* ApplicationContext::GetAllRunning(
    ApplicationManager* manager) {
  // TODO(xiang): We can't call app_manager_foreach_app_context here as it's
  // implemented on AUL API. So xwalk-launcher needs provide such info later.
  return NULL;
}

ApplicationContext::ApplicationContext(ApplicationManager* manager,
                                       const std::string& context_id)
    : err_code_(WebApiAPIErrors::NO_ERROR),
      context_id_(context_id) {
  int pid = std::stoi(context_id);
  if (pid <= 0)
    err_code_ = WebApiAPIErrors::NOT_FOUND_ERR;

  // We can't call app_manager_get_app_id here as it's implemented on
  // aul_app_get_appid_bypid which is an AUL API. So let xwalk-launcher handle
  // this for us.
  manager->GetAppIdByPid(pid, app_id_, err_code_);
}

ApplicationContext::~ApplicationContext() {
}

std::string ApplicationContext::Serialize() const {
  picojson::value result(picojson::object_type, true);
  picojson::object obj;
  if (!IsValid()) {
    obj["code"] = picojson::value(static_cast<double>(err_code_));
    result.get<picojson::object>()["error"] = picojson::value(obj);
  } else {
    obj["id"] = picojson::value(context_id_);
    obj["appId"] = picojson::value(app_id_);
    result.get<picojson::object>()["data"] = picojson::value(obj);
  }
  return result.serialize();
}

bool ApplicationContext::IsValid() const {
  return err_code_ == WebApiAPIErrors::NO_ERROR;
}

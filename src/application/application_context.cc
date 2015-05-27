// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "application/application_context.h"

#include <app_manager.h>
#include <aul.h>
#include <unistd.h>

#include <memory>
#include <utility>

#include "application/application_extension_utils.h"

namespace {

bool GetAppContextCallback(app_context_h app_context, void* user_data) {
  int pid;
  picojson::array* data = static_cast<picojson::array*>(user_data);
  if (app_context_get_pid(app_context, &pid) != APP_MANAGER_ERROR_NONE) {
    std::cerr << "Fail to get pid from context.\n";
    data->clear();
    return false;
  }

  ApplicationContext app_ctx(std::to_string(pid));
  if (!app_ctx.IsValid()) {
    data->clear();
    return false;
  }

  data->push_back(app_ctx.Value());
  return true;
}

}  // namespace

picojson::value* ApplicationContext::GetAllRunning() {
  picojson::array contexts;
  if (app_manager_foreach_app_context(GetAppContextCallback, &contexts)
      != APP_MANAGER_ERROR_NONE) {
    std::cerr << "Fail to call app_manager_foreach_app_context.\n";
    return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);
  }

  if (contexts.empty())
    return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);

  return CreateResultMessage(contexts);
}

ApplicationContext::ApplicationContext(const std::string& context_id)
    : error_(WebApiAPIErrors::NO_ERROR),
      context_id_(context_id) {
  int pid = std::stoi(context_id);
  if (pid <= 0)
    error_ = WebApiAPIErrors::NOT_FOUND_ERR;

  char* app_id = NULL;
  int ret = app_manager_get_app_id(pid, &app_id);
  if (ret == APP_MANAGER_ERROR_NONE && app_id) {
    app_id_ = app_id;
    free(app_id);
    return;
  }

  if (app_id)
    free(app_id);
  std::cerr << "Fail to get app id by pid: " << ret << std::endl;
  switch (ret) {
    case APP_MANAGER_ERROR_NO_SUCH_APP:
    case APP_MANAGER_ERROR_INVALID_PARAMETER:
      error_ = WebApiAPIErrors::NOT_FOUND_ERR;
      break;
    default:
      error_ = WebApiAPIErrors::UNKNOWN_ERR;
      break;
  }
}

const picojson::value& ApplicationContext::Value() {
  if (value_.is<picojson::null>() && IsValid()) {
    picojson::object obj;
    obj["id"] = picojson::value(context_id_);
    obj["appId"] = picojson::value(app_id_);
    value_ = picojson::value(obj);
  }
  return value_;
}

const std::string ApplicationContext::Serialize() {
  std::unique_ptr<picojson::value> result;
  if (!IsValid())
    result.reset(CreateResultMessage(error_));
  else
    result.reset(CreateResultMessage(Value()));
  return result->serialize();
}

bool ApplicationContext::IsValid() const {
  return error_ == WebApiAPIErrors::NO_ERROR;
}

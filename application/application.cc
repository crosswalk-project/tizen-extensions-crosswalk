// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "application/application.h"

#include <sys/types.h>
#include <unistd.h>
#include <utility>

#include "application/application_context.h"
#include "application/application_extension_utils.h"
#include "application/application_information.h"

Application::Application(const std::string& pkg_id)
    : pkg_id_(pkg_id) {
}

Application::~Application() {
}

ApplicationInformation Application::GetAppInfo() {
  if (app_id_.empty() && !RetrieveAppId())
    // Return an invalid ApplicationInformation by passing an empty app ID.
    return ApplicationInformation("");

  return ApplicationInformation(app_id_);
}

ApplicationContext Application::GetAppContext() {
  return ApplicationContext(std::to_string(getpid()));
}

picojson::value* Application::Exit() {
  // TODO(xiang): calls "Terminate" method of running app object on session bus.
  std::cerr << "ASSERT NOT IMPLEMENTED.\n";
  return CreateResultMessage();
}

picojson::value* Application::Hide() {
  // TODO(xiang): calls "Hide" method of running app object on session bus.
  std::cerr << "ASSERT NOT IMPLEMENTED.\n";
  return CreateResultMessage();
}

const std::string Application::Serialize() {
  ApplicationInformation app_info = GetAppInfo();
  ApplicationContext app_ctx = GetAppContext();

  std::unique_ptr<picojson::value> result;
  if (!app_info.IsValid() || !app_ctx.IsValid()) {
    result.reset(CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR));
  } else {
    picojson::object obj;
    obj["appInfo"] = app_info.Value();
    obj["appContext"] = app_ctx.Value();
    result.reset(CreateResultMessage(obj));
  }
  return result->serialize();
}

bool Application::RetrieveAppId() {
  app_id_ = ApplicationInformation::PkgIdToAppId(pkg_id_);
  if (app_id_.empty()) {
    std::cerr << "Can't translate app package ID to application ID.\n";
    return false;
  }
  return true;
}

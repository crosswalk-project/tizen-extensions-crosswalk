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

namespace {

const char kRuntimeServiceName[] =  "org.crosswalkproject.Runtime1";
const char kRuntimeRunningManagerPath[] = "/running1";
const char kRuntimeRunningAppInterface[] =
    "org.crosswalkproject.Running.Application1";

// The runtime process exports object for each running app on the session bus.
GDBusProxy* CreateRunningAppProxy(const std::string& xwalk_app_id) {
  GError* error = NULL;
  GDBusConnection* connection =
      g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
  if (!connection) {
    std::cerr << "Couldn't get the session bus connection: "
              << error->message << std::endl;
    g_error_free(error);
    return NULL;
  }

  std::string path =
      std::string(kRuntimeRunningManagerPath) + "/" + xwalk_app_id;
  GDBusProxy* proxy = g_dbus_proxy_new_sync(
      connection, G_DBUS_PROXY_FLAGS_NONE, NULL, kRuntimeServiceName,
      path.c_str(), kRuntimeRunningAppInterface, NULL, &error);
  if (!proxy) {
    std::cerr << "Couldn't create proxy for " << kRuntimeRunningAppInterface
              << ": " << error->message << std::endl;
    g_error_free(error);
    return NULL;
  }

  return proxy;
}

}  // namespace

Application::Application(const std::string& pkg_id)
    : pkg_id_(pkg_id),
      running_app_proxy_(NULL) {
}

Application::~Application() {
  if (running_app_proxy_)
    g_object_unref(running_app_proxy_);
}


const std::string Application::GetAppId() {
  if (app_id_.empty() && !RetrieveAppId())
    return "";
  return app_id_;
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
  if (!running_app_proxy_) {
    if (!(running_app_proxy_ = CreateRunningAppProxy(pkg_id_)))
      return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);
  }

  GError* error = NULL;
  GVariant* result = g_dbus_proxy_call_sync(
      running_app_proxy_, "Terminate", NULL,
      G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
  if (!result) {
    std::cerr << "Fail to call 'Terminate':" << error->message << std::endl;
    g_error_free(error);
    return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);
  }
  return CreateResultMessage();
}

picojson::value* Application::Hide() {
  if (!running_app_proxy_) {
    if (!(running_app_proxy_ = CreateRunningAppProxy(pkg_id_)))
      return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);
  }

  GError* error = NULL;
  GVariant* result = g_dbus_proxy_call_sync(
      running_app_proxy_, "Hide", NULL,
      G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
  if (!result) {
    std::cerr << "Fail to call 'Hide':" << error->message << std::endl;
    g_error_free(error);
    return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);
  }
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

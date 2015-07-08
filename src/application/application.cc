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
GDBusProxy* CreateRunningAppProxy(const std::string& app_id) {
  GError* error = NULL;
  GDBusConnection* connection =
      g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
  if (!connection) {
    std::cerr << "Couldn't get the session bus connection: "
              << error->message << std::endl;
    g_error_free(error);
    return NULL;
  }

  std::string path(app_id);
  // The [tizen_app_id] contains a dot, making it an invalid object path.
  // For this reason we replace it with an underscore '_'.
  std::replace(path.begin(), path.end(), '.', '_');
  path = std::string(kRuntimeRunningManagerPath) + "/" + path;

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

Application::Application(const std::string& app_id)
    : app_id_(app_id),
      running_app_proxy_(NULL) {
}

Application::~Application() {
  if (running_app_proxy_)
    g_object_unref(running_app_proxy_);
}


const std::string Application::GetAppId() {
  return app_id_;
}

ApplicationInformation Application::GetAppInfo() {
  return ApplicationInformation(app_id_);
}

ApplicationContext Application::GetAppContext() {
  return ApplicationContext(std::to_string(getpid()));
}

picojson::value* Application::Exit() {
  if (!running_app_proxy_) {
    if (!(running_app_proxy_ = CreateRunningAppProxy(app_id_)))
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
  g_variant_unref(result);
  return CreateResultMessage();
}

picojson::value* Application::Hide() {
  if (!running_app_proxy_) {
    if (!(running_app_proxy_ = CreateRunningAppProxy(app_id_)))
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
  g_variant_unref(result);
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

// Copyright (c) 2014 Samsung Electronics Co., Ltd. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "web_setting/web_setting.h"

#include <sys/types.h>
#include <unistd.h>
#include <utility>

#include "web_setting/web_setting_extension_utils.h"

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

  std::string path =
      std::string(kRuntimeRunningManagerPath) + "/" + app_id;
  // Every application id contains '.' character and since object path
  // is created from application id it also contains '.' character.
  // The d-bus proxy doesn't accept '.' character in object path
  // And that is why the substantiation is needed here.
  std::replace(path.begin(), path.end(), '.', '_');
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

WebSetting::WebSetting(const std::string& app_id)
    : app_id_(app_id),
      running_app_proxy_(NULL) {
}

WebSetting::~WebSetting() {
  if (running_app_proxy_)
    g_object_unref(running_app_proxy_);
}

std::unique_ptr<picojson::value> WebSetting::RemoveAllCookies() {
  if (!running_app_proxy_) {
    if (!(running_app_proxy_ = CreateRunningAppProxy(app_id_)))
      return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);
  }
  GError* error = NULL;
  GVariant* result = g_dbus_proxy_call_sync(
      running_app_proxy_, "RemoveAllCookies", NULL,
      G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
  if (!result) {
    std::cerr << "Fail to call 'RemoveuserAgentAllCookies':"
              << error->message << std::endl;
    g_error_free(error);
    return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);
  }
  g_variant_unref(result);
  return CreateResultMessage();
}

std::unique_ptr<picojson::value> WebSetting::SetUserAgentString(
    const std::string& user_agent) {
  if (!running_app_proxy_) {
    if (!(running_app_proxy_ = CreateRunningAppProxy(app_id_)))
      return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);
  }
  GError* error = NULL;
  GVariant* result = g_dbus_proxy_call_sync(
      running_app_proxy_, "SetUserAgentString",
      g_variant_new("(s)", user_agent.c_str()),
      G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
  if (!result) {
    std::cerr << "Fail to call 'SetUserAgentString':"
              << error->message << std::endl;
    g_error_free(error);
    return CreateResultMessage(WebApiAPIErrors::UNKNOWN_ERR);
  }
  g_variant_unref(result);
  return CreateResultMessage();
}

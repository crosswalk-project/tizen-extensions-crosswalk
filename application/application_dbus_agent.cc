// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "application/application_dbus_agent.h"

#include <string>

namespace {

const char kRuntimeServiceName[] =  "org.crosswalkproject.Runtime1";

const char kRuntimeRunningManagerPath[] = "/running1";

const char kRuntimeRunningAppInterface[] =
    "org.crosswalkproject.Running.Application1";

const char kLauncherDBusObjectPath[] = "/launcher1";

const char kLauncherDBusApplicationInterface[] =
    "org.crosswalkproject.Launcher.Application1";

// Create proxy for running application on runtime process. The runtime process
// exports its object on session message bus.
GDBusProxy* ConnectToRuntimeRunningApp(GDBusConnection* connection,
                                       const std::string& xwalk_app_id) {
  GError* error = NULL;
  std::string path = kRuntimeRunningManagerPath;
  path += "/" + xwalk_app_id;
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

// Create proxy for application interface exported by xwalk-launcher.
GDBusProxy* ConnectToLauncherApp(GDBusConnection* connection,
                                 GDBusProxy* runtime_app_proxy) {
  // xwalk-launcher has no well-known name on the session bus. So we fetch its
  // unique name from runtime app proxy property.
  const gchar* launcher_name;
  GVariant* value = g_dbus_proxy_get_cached_property(
      runtime_app_proxy, "LauncherName");
  g_variant_get(value, "&s", &launcher_name);

  GError* error = NULL;
  GDBusProxy* proxy = g_dbus_proxy_new_sync(
      connection, G_DBUS_PROXY_FLAGS_NONE, NULL, launcher_name,
      kLauncherDBusObjectPath, kLauncherDBusApplicationInterface, NULL, &error);
  g_variant_unref(value);
  if (!proxy) {
    std::cerr << "Couldn't create proxy for "
              << kLauncherDBusApplicationInterface
              << ": " << error->message << std::endl;
    g_error_free(error);
    return NULL;
  }

  return proxy;
}

void FinishAppOperation(GObject* source_object,
                        GAsyncResult* res,
                        gpointer user_data) {
  GError* error = NULL;
  GVariant* value;
  GDBusProxy* proxy = G_DBUS_PROXY(source_object);
  AppDBusCallback* callback = static_cast<AppDBusCallback*>(user_data);

  value = g_dbus_proxy_call_finish(proxy, res, &error);
  if (!value) {
    std::cerr << "Failed to run app operation:" << error->message << std::endl;
    g_error_free(error);
  }

  (*callback)(value);
  if (value)
    g_variant_unref(value);
  delete callback;
}

}  // namespace

ApplicationDBusAgent* ApplicationDBusAgent::Create(
    const std::string& xwalk_app_id) {
  GError* error = NULL;
  GDBusConnection* connection =
      g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
  if (!connection) {
    std::cerr << "Couldn't get the session bus connection: "
              << error->message << std::endl;
    g_error_free(error);
    return NULL;
  }

  GDBusProxy* runtime_app_proxy =
      ConnectToRuntimeRunningApp(connection, xwalk_app_id);
  if (!runtime_app_proxy)
    return NULL;

  GDBusProxy* launcher_app_proxy =
      ConnectToLauncherApp(connection, runtime_app_proxy);
  if (!launcher_app_proxy)
    return NULL;

  return new ApplicationDBusAgent(runtime_app_proxy, launcher_app_proxy);
}

ApplicationDBusAgent::ApplicationDBusAgent(GDBusProxy* runtime_app_proxy,
                                           GDBusProxy* launcher_app_proxy)
    : runtime_proxy_(runtime_app_proxy),
      launcher_proxy_(launcher_app_proxy) {
}

GVariant* ApplicationDBusAgent::GetLauncherPid() {
  GError* error = NULL;
  GVariant* result = g_dbus_proxy_call_sync(launcher_proxy_, "GetPid",
                                            NULL, G_DBUS_CALL_FLAGS_NONE,
                                            -1, NULL, &error);
  if (!result) {
    std::cerr << "Couldn't call 'GetPid' method:"
              << error->message << std::endl;
    g_error_free(error);
    return NULL;
  }
  return result;
}

GVariant* ApplicationDBusAgent::GetAppIdByPid(int pid) {
  GError* error = NULL;
  GVariant* result = g_dbus_proxy_call_sync(
      launcher_proxy_, "GetAppIdByPid", g_variant_new("(i)", pid),
      G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
  if (!result) {
    std::cerr << "Couldn't call 'GetAppIdByPid' method:"
              << error->message << std::endl;
    g_error_free(error);
    return NULL;
  }
  return result;
}

GVariant* ApplicationDBusAgent::ExitCurrentApp() {
  // Currently xwalk runtime only allow the caller of Launch() to call
  // Terminate(), so we need let xwalk-laucher to do this job.
  GError* error = NULL;
  GVariant* result = g_dbus_proxy_call_sync(launcher_proxy_, "Exit",
                                            NULL, G_DBUS_CALL_FLAGS_NONE,
                                            -1, NULL, &error);
  if (!result) {
    std::cerr << "Couldn't call 'Exit' method:" << error->message << std::endl;
    g_error_free(error);
    return NULL;
  }
  return result;
}

GVariant* ApplicationDBusAgent::HideCurrentApp() {
  GError* error = NULL;
  GVariant* result = g_dbus_proxy_call_sync(
      runtime_proxy_, "Hide", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
  if (!result) {
    std::cerr << "Couldn't call 'Hide' method:" << error->message << std::endl;
    g_error_free(error);
    return NULL;
  }
  return result;
}

void ApplicationDBusAgent::LaunchApp(const std::string& app_id,
                                     AppDBusCallback callback) {
  AppDBusCallback* saved_callback = new AppDBusCallback(callback);
  g_dbus_proxy_call(launcher_proxy_, "LaunchApp", g_variant_new("(s)",
                    app_id.c_str()), G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                    FinishAppOperation, saved_callback);
}

void ApplicationDBusAgent::KillApp(const std::string& context_id,
                                   AppDBusCallback callback) {
  AppDBusCallback* saved_callback = new AppDBusCallback(callback);
  g_dbus_proxy_call(launcher_proxy_, "KillApp", g_variant_new("(s)",
                    context_id.c_str()), G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                    FinishAppOperation, saved_callback);
}

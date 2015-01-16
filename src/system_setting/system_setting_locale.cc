// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_setting/system_setting_locale.h"

#include <gio/gio.h>
#include <iostream>

#define LOGGER_E(msg) std::cerr << "\n[Error] " << msg << std::endl;
#define LOGGER_D(msg) std::cerr << "\n[DEBUG] " << msg << std::endl;

namespace {

const char sDBusProp[] = "org.freedesktop.DBus.Properties";
const char sInterfaceName[] = "org.freedesktop.locale1";
const char sObjectPath[] = "/org/freedesktop/locale1";

}  // anonymous namespace

namespace system_setting {

std::string getLocale() {
  GError* error = nullptr;

  GDBusProxy* proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                    G_DBUS_PROXY_FLAGS_NONE,
                                                    NULL,
                                                    sInterfaceName,
                                                    sObjectPath,
                                                    sDBusProp,
                                                    NULL,
                                                    &error);

  GVariant* property = g_dbus_proxy_call_sync(proxy,
                                              "Get",
                                              g_variant_new("(ss)",
                                                            sInterfaceName,
                                                            "Locale"),
                                              G_DBUS_CALL_FLAGS_NONE,
                                              -1,
                                              NULL,
                                              &error);

  g_object_unref(proxy);

  if (error)  {
    LOGGER_E("Error getting locale: " << error->message);
    g_error_free(error);
    return NULL;
  }

  GVariant* local_var;

  g_variant_get(property, "(v)", &local_var);
  LOGGER_D("locale type string: " << g_variant_get_type_string(local_var));

  GVariantIter* iter;
  g_variant_get(local_var, "as", &iter);
  char* locale;
  g_variant_iter_next(iter, "s", &locale);

  LOGGER_D("system locale is: " << locale);

  std::string sublocale = std::string(locale).substr(5);
  g_free(locale);

  if (local_var)
    g_variant_unref(local_var);

  if (property)
    g_variant_unref(property);

  if (iter)
    g_variant_iter_free(iter);

  return sublocale;
}

void setLocale(const std::string& locale_str) {
  LOGGER_D("Trying to set locale to: " << locale_str);
  GError* error = NULL;
  GDBusProxy* proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                    G_DBUS_PROXY_FLAGS_NONE,
                                                    NULL,
                                                    sInterfaceName,
                                                    sObjectPath,
                                                    sInterfaceName,
                                                    NULL,
                                                    &error);

  GVariantBuilder builder;
  g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
  g_variant_builder_add(&builder, "s", locale_str.c_str());
  GVariant* val = g_dbus_proxy_call_sync(proxy,
                                         "SetLocale",
                                         g_variant_new("(asb)",
                                                       &builder,
                                                       false),
                                         G_DBUS_CALL_FLAGS_NONE,
                                         -1,
                                         NULL,
                                         &error);

  if (error)  {
    LOGGER_E("Error setting locale: " << error->message);
    g_error_free(error);
  }

  if (val)
    g_variant_unref(val);

  g_object_unref(proxy);
}
}  // namespace system_setting

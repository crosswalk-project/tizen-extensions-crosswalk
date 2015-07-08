// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_utils.h"

#include <stdio.h>
#if defined(TIZEN)
#include <tzplatform_config.h>
#endif
#include <unistd.h>

#include <algorithm>
#include <fstream>

namespace {

const int kDuidBufferSize = 100;
const char kDuidStrKey[] = "http://tizen.org/system/duid";

}  // namespace

namespace system_info {

#ifdef TIZEN
char* GetDuidProperty() {
  char* s = NULL;
  FILE* fp = NULL;
  static char duid[kDuidBufferSize] = {0, };
  size_t len = strlen(kDuidStrKey);
  fp = fopen(tzplatform_mkpath(TZ_USER_ETC, "system_info_cache.ini"), "r");

  if (fp) {
    while (fgets(duid, kDuidBufferSize - 1, fp)) {
      if (strncmp(duid, kDuidStrKey, len) == 0) {
        char* token = NULL;
        char* ptr = NULL;
        token = strtok_r(duid, "=\r\n", &ptr);
        if (token != NULL) {
            token = strtok_r(NULL, "=\r\n", &ptr);
            if (token != NULL)
              s = token;
        }
        break;
      }
    }
    fclose(fp);
  }
  return s;
}

#ifndef TIZEN_MOBILE
GDBusConnection* GetDbusConnection() {
  GError* error = NULL;
  gchar* addr = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                NULL, &error);
  if (!addr) {
    std::cout << "fail to get dbus addr: " << error->message << std::endl;
    g_free(error);
    return NULL;
  }

  GDBusConnection* bus_conn = g_dbus_connection_new_for_address_sync(addr,
      (GDBusConnectionFlags)(G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
      G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION), NULL, NULL, &error);
  if (!bus_conn) {
    std::cout << "fail to create dbus connection: "
              << error->message << std::endl;
    g_free(error);
    return NULL;
  }
  return bus_conn;
}

std::string OfonoGetModemPath(GDBusConnection* bus_conn) {
  GError* error = NULL;
  GVariant* modems_result = g_dbus_connection_call_sync(bus_conn,
      kOfonoService, kOfonoManagerPath, kOfonoManagerIface,
      "GetModems", NULL, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

  if (!modems_result) {
    std::cout << "ofono GetModems failed: " << error->message << std::endl;
    g_error_free(error);
    return std::string("");
  }

  std::string result_path;
  GVariantIter* modems_iter;
  g_variant_get(modems_result, "(a(oa{sv}))", &modems_iter);
  g_variant_unref(modems_result);

  gchar* path;
  GVariant* modem_properties;
  while (g_variant_iter_next(modems_iter, "(o@a{sv})",
                             &path, &modem_properties)) {
    if (!path)
      continue;

    GVariantIter* properties_iter;
    g_variant_get(modem_properties, "a{sv}", &properties_iter);
    g_variant_unref(modem_properties);

    bool hardware_flag = false;
    gchar* key;
    GVariant* var_val;
    while (g_variant_iter_next(properties_iter, "{sv}", &key, &var_val) &&
           !hardware_flag) {
      if (g_strcmp0(key, "Type") == 0) {
        const char* modem_type = g_variant_get_string(var_val, NULL);
        if (g_strcmp0(modem_type, "hardware") == 0) {
          result_path = std::string(path);
          hardware_flag = true;
        } else if (g_strcmp0(modem_type, "sap") == 0) {
          result_path = std::string(path);
        }
      }
      g_free(key);
      g_variant_unref(var_val);
    }
    g_free(path);
    g_variant_iter_free(properties_iter);
    if (hardware_flag)
      break;
  }
  g_variant_iter_free(modems_iter);
  return result_path;
}
#endif  // TIZEN_MOBILE
#endif  // TIZEN

int ReadOneByte(const char* path) {
  FILE* fp = fopen(path, "r");

  if (!fp)
    return -1;

  int ret = fgetc(fp);
  fclose(fp);

  return ret;
}

char* ReadOneLine(const char* path) {
  FILE* fp = fopen(path, "r");
  char* line = NULL;
  size_t len = 0;
  ssize_t read;

  if (NULL == fp)
    return NULL;

  read = getline(&line, &len, fp);
  if (-1 == read)
    return NULL;

  fclose(fp);

  return line;
}

std::string GetUdevProperty(struct udev_device* dev,
                            const std::string& attr) {
  struct udev_list_entry *attr_list_entry, *attr_entry;

  attr_list_entry = udev_device_get_properties_list_entry(dev);
  attr_entry = udev_list_entry_get_by_name(attr_list_entry, attr.c_str());
  if (!attr_entry)
    return "";

  return std::string(udev_list_entry_get_value(attr_entry));
}

void SetPicoJsonObjectValue(picojson::value& obj,
                            const char* prop,
                            const picojson::value& val) {
  picojson::object& o = obj.get<picojson::object>();
  o[prop] = val;
}

std::string GetPropertyFromFile(const std::string& file_path,
                                const std::string& key) {
  std::ifstream in(file_path.c_str());
  if (!in)
    return "";

  std::string line;
  while (getline(in, line)) {
    line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());

    if (line.substr(0, line.find("=")) == key) {
      return line.substr(line.find("=") + 1, line.length());
    }
  }

  return "";
}

}  // namespace system_info

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_utils.h"

#include <stdio.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>

namespace {

const int kDuid_buffer_size = 100;
const char kDuid_str_key[] = "http://tizen.org/system/duid";

}  // namespace

namespace system_info {

char* GetDuidProperty() {
  char *s = NULL;
  FILE *fp = NULL;
  static char duid[kDuid_buffer_size] = {0, };
  size_t len = strlen(kDuid_str_key);
  fp = fopen("/opt/usr/etc/system_info_cache.ini", "r");

  if (fp) {
    while (fgets(duid, kDuid_buffer_size - 1, fp)) {
      if (strncmp(duid, kDuid_str_key, len) == 0) {
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
  char *line = NULL;
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

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_utils.h"

#include <stdio.h>

namespace system_info {

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
  if (0 == attr_entry)
    return NULL;

  return std::string(udev_list_entry_get_value(attr_entry));
}

void SetPicoJsonObjectValue(picojson::value& obj,
                            const char* prop,
                            const picojson::value& val) {
  picojson::object& o = obj.get<picojson::object>();
  o[prop] = val;
}

}  // namespace system_info

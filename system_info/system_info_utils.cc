// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_utils.h"

#include <stdio.h>

#define SYS_CLASS_NET_DIR "/sys/class/net"
#define MAXBUFSIZE 512

namespace system_info {

bool is_interface_on(const char* interface) {
  char path[MAXBUFSIZE];
  sprintf(path, "%s/%s/%s", SYS_CLASS_NET_DIR, interface, "carrier");

  int if_on = read_one_byte(path);

  return (49 == if_on);  // ascii code for '1'
}

int read_one_byte(const char* path) {
  FILE* fp = fopen(path, "r");

  if (!fp)
    return -1;

  int ret = fgetc(fp);
  fclose(fp);

  return ret;
}

char* read_one_line(const char* path) {
  FILE* fp = fopen(path, "r");
  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  if (NULL == fp)
    return NULL;

  read = getline(&line, &len, fp);
  if(-1 == read)
    return NULL;

  fclose(fp);

  return line;
}

std::string get_udev_property(struct udev_device* dev,
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

}  // system_info

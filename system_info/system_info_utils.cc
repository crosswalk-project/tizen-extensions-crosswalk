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

}  // system_info

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_utils.h"

#include <stdio.h>

#define SYS_CLASS_NET_DIR "/sys/class/net"
#define MAXBUFSIZE 512

namespace system_info {

bool is_interface_on(const char* interface) {
  FILE* f = NULL;
  char path[MAXBUFSIZE];
  sprintf(path, "%s/%s/%s", SYS_CLASS_NET_DIR, interface, "carrier");

  f = fopen(path, "r");
  int if_on = fgetc(f);
  fclose(f);

  return (49 == if_on);  // ascii code for '1'
}

}  // system_info

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "download/download_instance.h"

#include <pwd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

std::string DownloadInstance::GetFullDestinationPath(
    const std::string destination) const {
  std::string path(getenv("HOME"));
  if (path.empty()) {
    struct passwd password_entry;
    struct passwd* password_entry_ptr;
    char buf[1024];
    if (!getpwuid_r(getuid(), &password_entry,
                    buf, sizeof buf, &password_entry_ptr)) {
      path = password_entry.pw_dir;
    }
  }
  return (path + '/' + destination);
}

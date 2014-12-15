// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "download/download_instance.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <tzplatform_config.h>
#include <unistd.h>

const std::string DownloadInstance::GetFullDestinationPath(
    const std::string destination) const {
  std::string real_path;
  if (destination.empty())
    real_path = vfs_.GetRealPath(vfs_const::kLocationDownloads);
  else
    real_path = vfs_.GetRealPath(destination);
  return real_path;
}

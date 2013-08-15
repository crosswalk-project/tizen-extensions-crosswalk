// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "download/download_context.h"

std::string DownloadContext::GetFullDestinationPath(
    const std::string destination) const {
  std::string folder = (destination == "null") ? std::string() : destination;

  // TODO(hdq): User should be able to choose store to external storage
  //            i.e. /opt/storage/sdcard/Downloads
  std::string path("/opt/usr/media/");
  if (folder.empty()) {
    return (path + "Downloads/");
  }
  return (path + folder);
}

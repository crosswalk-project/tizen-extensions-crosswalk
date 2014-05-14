// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sys/types.h>
#include <sys/stat.h>
#include <tzplatform_config.h>
#include <unistd.h>

#include "download/download_context.h"

const std::string DownloadContext::GetActualFolder(
    const std::string& destination) const {
  typedef std::map<std::string, std::string> LocationMap;
  static const LocationMap::value_type data[] = {
     LocationMap::value_type("documents", "Documents"),
     LocationMap::value_type("downloads", "Downloads"),
     LocationMap::value_type("images", "Images"),
     LocationMap::value_type("music", "Sounds"),
     LocationMap::value_type("videos", "Videos"),
  };
  static const LocationMap locations(data, data + sizeof data / sizeof data[0]);
  LocationMap::const_iterator location = locations.find(destination);
  if (location == locations.end()) {
    return destination;
  } else {
    return location->second;
  }
}

const std::string DownloadContext::GetFullDestinationPath(
    const std::string destination) const {
  // TODO(hdq): User should be able to choose store to external storage
  //            i.e. /opt/storage/sdcard/Downloads
  const std::string default_folder("Downloads");
  const std::string location = destination.empty() ? default_folder : destination;
  std::string path = tzplatform_getenv(TZ_USER_CONTENT) + GetActualFolder(location);

  // Create path if not exist
  struct stat path_stat;
  if (stat(path.c_str(), &path_stat) == -1
      && mkdir(path.c_str(), 0777) != 0) {
    path = tzplatform_getenv(TZ_USER_CONTENT) + default_folder;
  }

  return path;
}

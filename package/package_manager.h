// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PACKAGE_PACKAGE_MANAGER_H_
#define PACKAGE_PACKAGE_MANAGER_H_

#include <package_manager.h>

#include <map>
#include <string>

#include "package/package_instance.h"

class PackageManager {
 public:
  explicit PackageManager(PackageInstance* instance);
  ~PackageManager();
  void PackageManagerDestroy();

  picojson::value* InstallPackage(const char* pkg_path, double request_id);
  picojson::value* UnInstallPackage(const char* pkg_id, double request_id);
  picojson::value* RegisterPackageInfoEvent();
  picojson::value* UnregisterPackageInfoEvent();

  void OnPackageInfoEvent(const std::string& pkg_id, int event_type);
  void OnPackageRequestCallback(
      int id, const std::string& pkg_id,
      int event_type, int event_state,
      int progress);

 private:
  PackageInstance* instance_;
  package_manager_h manager_;
  package_manager_request_h request_handle_;
  std::map<int, double> callbacks_id_map_;
};

#endif  // PACKAGE_PACKAGE_MANAGER_H_

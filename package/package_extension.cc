// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <sstream>

#include "common/picojson.h"
#include "package/package_extension.h"
#include "package/package_instance.h"

common::Extension* CreateExtension() {
  std::string id_str = common::Extension::GetRuntimeVariable("app_id", 64);
  picojson::value id_val;
  std::istringstream buf(id_str);
  std::string error = picojson::parse(id_val, buf);
  if (!error.empty()) {
    std::cerr << "Got invalid application ID." << std::endl;
    return NULL;
  }

  std::string app_id = id_val.get<std::string>();
  if (app_id.empty()) {
    std::cerr << "Package extension will not be created without "
              << "context." << std::endl;
    return NULL;
  }

  return new PackageExtension(app_id);
}

// JS source code for the API
extern const char kSource_package_api[];

PackageExtension::PackageExtension(const std::string& app_id)
    : app_id_(app_id) {
  SetExtensionName("tizen.package");
  SetJavaScriptAPI(kSource_package_api);
}

PackageExtension::~PackageExtension() {}

common::Instance* PackageExtension::CreateInstance() {
  return new PackageInstance(this);
}

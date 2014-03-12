// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "application/application_extension.h"

#include <iostream>
#include <sstream>

#include "application/application_manager.h"
#include "application/application_instance.h"
#include "common/picojson.h"

common::Extension* CreateExtension() {
  // NOTE: The app_id here is package ID instead of application ID for Tizen.
  // As Crosswalk app has 1 to 1 mapping between package and application, we
  // can transfer pkg_id to app_id.
  std::string id_str = common::Extension::GetRuntimeVariable("app_id", 64);
  picojson::value id_val;
  std::istringstream buf(id_str);
  std::string error = picojson::parse(id_val, buf);
  if (!error.empty()) {
    std::cerr << "Got invalid application ID." << std::endl;
    return NULL;
  }

  std::string pkg_id = id_val.get<std::string>();
  if (pkg_id.empty()) {
    std::cerr << "Application extension will not be created without "
              << "application context." << std::endl;
    return NULL;
  }

  ApplicationManager* manager = ApplicationManager::Create(pkg_id);
  if (!manager) {
    std::cerr << "Can't create application interface." << std::endl;
    return NULL;
  }

  return new ApplicationExtension(manager);
}

// This will be generated from application_api.js
extern const char kSource_application_api[];

ApplicationExtension::ApplicationExtension(ApplicationManager* manager)
    : manager_(manager) {
  assert(manager_);
  SetExtensionName("tizen.application");
  SetJavaScriptAPI(kSource_application_api);
}

ApplicationExtension::~ApplicationExtension() {}

common::Instance* ApplicationExtension::CreateInstance() {
  return new ApplicationInstance(manager_.get());
}

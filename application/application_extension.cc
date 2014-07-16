// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "application/application_extension.h"

#include <iostream>
#include <sstream>

#include "application/application.h"
#include "application/application_instance.h"
#include "application/application_manager.h"
#include "common/picojson.h"

common::Extension* CreateExtension() {
  // For xpk, tizen_app_id = xwalk.package_id;
  //          package_id = crosswalk_32bytes_app_id;
  // For wgt, tizen_app_id = package_id.app_name,
  //          package_id = tizen_wrt_10bytes_package_id;
  std::string id_str =
      common::Extension::GetRuntimeVariable("tizen_app_id", 64);
  picojson::value id_val;
  std::istringstream buf(id_str);
  std::string error = picojson::parse(id_val, buf);
  if (!error.empty()) {
    std::cerr << "Got invalid application ID." << std::endl;
    return NULL;
  }

  std::string tizen_app_id = id_val.get<std::string>();
  if (tizen_app_id.empty()) {
    std::cerr << "Application extension will not be created without "
              << "application context." << std::endl;
    return NULL;
  }

  std::string pkg_id;
  if (tizen_app_id.find("xwalk.") == 0)
    pkg_id = tizen_app_id.substr(6);
  else
    pkg_id = tizen_app_id.substr(0, 10);

  return new ApplicationExtension(pkg_id);
}

// This will be generated from application_api.js
extern const char kSource_application_api[];

ApplicationExtension::ApplicationExtension(const std::string& pkg_id) {
  current_app_.reset(new Application(pkg_id));
  app_manager_.reset(new ApplicationManager());

  SetExtensionName("tizen.application");
  SetJavaScriptAPI(kSource_application_api);
}

ApplicationExtension::~ApplicationExtension() {}

common::Instance* ApplicationExtension::CreateInstance() {
  return new ApplicationInstance(this);
}

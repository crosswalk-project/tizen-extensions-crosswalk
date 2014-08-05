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
    std::cerr << "Application extension will not be created without "
              << "application context." << std::endl;
    return NULL;
  }

  return new ApplicationExtension(app_id);
}

// This will be generated from application_api.js
extern const char kSource_application_api[];

ApplicationExtension::ApplicationExtension(const std::string& app_id) {
  current_app_.reset(new Application(app_id));
  app_manager_.reset(new ApplicationManager());

  SetExtensionName("tizen.application");
  SetJavaScriptAPI(kSource_application_api);
}

ApplicationExtension::~ApplicationExtension() {}

common::Instance* ApplicationExtension::CreateInstance() {
  return new ApplicationInstance(this);
}

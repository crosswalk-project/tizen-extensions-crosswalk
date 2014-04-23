// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPLICATION_APPLICATION_EXTENSION_H_
#define APPLICATION_APPLICATION_EXTENSION_H_

#include <memory>
#include <string>

#include "common/extension.h"

class Application;
class ApplicationManager;

class ApplicationExtension : public common::Extension {
 public:
  explicit ApplicationExtension(const std::string& pkg_id);
  virtual ~ApplicationExtension();

  Application* current_app() { return current_app_.get(); }
  ApplicationManager* app_manager() { return app_manager_.get(); }

 private:
  // common::Extension implementation.
  virtual common::Instance* CreateInstance();

  std::unique_ptr<Application> current_app_;
  std::unique_ptr<ApplicationManager> app_manager_;
};

#endif  // APPLICATION_APPLICATION_EXTENSION_H_

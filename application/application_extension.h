// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPLICATION_APPLICATION_EXTENSION_H_
#define APPLICATION_APPLICATION_EXTENSION_H_

#include <memory>
#include <string>

#include "common/extension.h"

class ApplicationManager;

class ApplicationExtension : public common::Extension {
 public:
  explicit ApplicationExtension(ApplicationManager* current_app);
  virtual ~ApplicationExtension();

 private:
  // common::Extension implementation.
  virtual common::Instance* CreateInstance();

  std::unique_ptr<ApplicationManager> manager_;
};

#endif  // APPLICATION_APPLICATION_EXTENSION_H_

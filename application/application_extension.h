// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPLICATION_APPLICATION_EXTENSION_H_
#define APPLICATION_APPLICATION_EXTENSION_H_

#include <string>

#include "common/extension.h"

class ApplicationExtension : public common::Extension {
 public:
  ApplicationExtension(const std::string& app_id, const std::string& pkg_id);
  virtual ~ApplicationExtension();

  const std::string& app_id() const { return app_id_; }

  const std::string& pkg_id() const { return pkg_id_; }

 private:
  // common::Extension implementation.
  virtual common::Instance* CreateInstance();

  std::string app_id_;
  std::string pkg_id_;
};

#endif  // APPLICATION_APPLICATION_EXTENSION_H_

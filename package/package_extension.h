// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PACKAGE_PACKAGE_EXTENSION_H_
#define PACKAGE_PACKAGE_EXTENSION_H_

#include <string>

#include "common/extension.h"

class PackageExtension : public common::Extension {
 public:
  explicit PackageExtension(const std::string& app_id);
  virtual ~PackageExtension();

  const std::string& current_app_id() { return app_id_; }

 private:
  virtual common::Instance* CreateInstance();
  std::string app_id_;
};

#endif  // PACKAGE_PACKAGE_EXTENSION_H_

// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PACKAGE_PACKAGE_INFO_H_
#define PACKAGE_PACKAGE_INFO_H_

#include <memory>
#include <string>

#include "common/picojson.h"

class PackageInformation {
 public:
  static picojson::value* GetAllInstalled();

  PackageInformation(const std::string& id, bool is_app_id);
  ~PackageInformation();

  const picojson::value& Value();
  const std::string Serialize();
  bool IsValid() const;

 private:
  picojson::object data_;
  picojson::object error_;
  picojson::value value_;
};

#endif  // PACKAGE_PACKAGE_INFO_H_

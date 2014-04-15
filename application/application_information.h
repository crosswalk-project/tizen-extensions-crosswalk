// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPLICATION_APPLICATION_INFORMATION_H_
#define APPLICATION_APPLICATION_INFORMATION_H_

#include <string>

#include "common/picojson.h"

// If application information is successfully retrieved, then the data_ value
// will contains the related info. Otherwise the error_ value will contains
// failure reason.
class ApplicationInformation {
 public:
  // Translate app's package ID to application ID, only valid for XWalk apps.
  static std::string PkgIdToAppId(const std::string& pkg_id);

  // Returns a object type value to caller. When success its "data" field will
  // contains an app info array. When fail its "error" field will contains error
  // info.
  static picojson::object* GetAllInstalled();

  explicit ApplicationInformation(const std::string& app_id);
  ~ApplicationInformation();

  std::string Serialize() const;

 private:
  bool IsValid() const;

  picojson::value data_;
  picojson::value error_;
};

#endif  // APPLICATION_APPLICATION_INFORMATION_H_

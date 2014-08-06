// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPLICATION_APPLICATION_INFORMATION_H_
#define APPLICATION_APPLICATION_INFORMATION_H_

#include <memory>
#include <string>

#include "common/picojson.h"

// If application information is successfully retrieved, then the data_ value
// will contains the related info. Otherwise the error_ value will contains
// failure reason.
class ApplicationInformation {
 public:
  // Returns a object type value to caller. When success its "data" field will
  // contains an app info array. When fail its "error" field will contains error
  // info.
  static picojson::value* GetAllInstalled();

  explicit ApplicationInformation(const std::string& app_id);
  ~ApplicationInformation();

  const picojson::value& Value();
  const std::string Serialize();
  bool IsValid() const;

 private:
  picojson::object data_;
  picojson::object error_;
  picojson::value value_;
};

#endif  // APPLICATION_APPLICATION_INFORMATION_H_

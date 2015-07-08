// Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPLICATION_APPLICATION_CONTROL_DATA_H_
#define APPLICATION_APPLICATION_CONTROL_DATA_H_

#include <memory>
#include <string>
#include <vector>

#include "common/picojson.h"
#include "common/utils.h"

class ApplicationControlData {
 public:
  ApplicationControlData(
      const std::string& key,
      std::unique_ptr<std::vector<std::string>> values);

  const std::string& key() const;
  const std::vector<std::string>& values() const;

  std::unique_ptr<picojson::value> ToJson() const;

  static std::unique_ptr<ApplicationControlData> ApplicationControlDataFromJSON(
      const picojson::value& value);

 private:
  std::string key_;
  std::unique_ptr<std::vector<std::string>> values_;

  DISALLOW_COPY_AND_ASSIGN(ApplicationControlData);
};

#endif  // APPLICATION_APPLICATION_CONTROL_DATA_H_

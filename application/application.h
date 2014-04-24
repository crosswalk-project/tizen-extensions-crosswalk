// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPLICATION_APPLICATION_H_
#define APPLICATION_APPLICATION_H_

#include <memory>
#include <string>

#include "common/picojson.h"

class ApplicationContext;
class ApplicationInformation;

// This class will represent the current application running this extension.
class Application {
 public:
  explicit Application(const std::string& pkg_id);
  ~Application();

  ApplicationInformation GetAppInfo();
  ApplicationContext GetAppContext();

  picojson::value* Exit();
  picojson::value* Hide();

  const std::string Serialize();

 private:
  bool RetrieveAppId();

  std::string app_id_;
  std::string pkg_id_;
};

#endif  // APPLICATION_APPLICATION_H_

// Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPLICATION_APPLICATION_REQUESTED_CONTROL_H_
#define APPLICATION_APPLICATION_REQUESTED_CONTROL_H_

#include <memory>
#include <string>

#include "application/application_control.h"
#include "common/picojson.h"
#include "common/utils.h"

class ApplicationRequestedControl {
 public:
  ApplicationRequestedControl(const std::string& caller_id,
      std::unique_ptr<ApplicationControl> app_control);

  const ApplicationControl* app_control() const;

  std::unique_ptr<picojson::value> ToJson() const;

  static std::unique_ptr<ApplicationRequestedControl>
      GetRequestedApplicationControl(const std::string& encoded_bundle);

 private:
  std::string caller_id_;
  std::unique_ptr<ApplicationControl> app_control_;

  DISALLOW_COPY_AND_ASSIGN(ApplicationRequestedControl);
};

#endif  // APPLICATION_APPLICATION_REQUESTED_CONTROL_H_

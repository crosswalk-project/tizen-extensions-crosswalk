// Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPLICATION_APPLICATION_CONTROL_H_
#define APPLICATION_APPLICATION_CONTROL_H_

#include <app_service.h>
#include <bundle.h>

#include <memory>
#include <string>
#include <vector>

#include "application/application_control_data.h"
#include "common/picojson.h"
#include "common/utils.h"

bool AddAppControlDataToService(const ApplicationControlData& data,
    service_h service);
bool AddAppControlDataArrayToService(
    const std::vector<std::unique_ptr<ApplicationControlData>>& array,
    service_h service);
bundle* DecodeApplicationBundle(const std::string& encoded_bundle);

class ApplicationControl {
 public:
  ApplicationControl(
      const std::string& operation,
      const std::string& uri,
      const std::string& mime,
      const std::string& category);

  const std::string& operation() const;
  const std::string& mime() const;
  const std::string& uri() const;
  const std::string& category() const;
  const std::vector<std::unique_ptr<ApplicationControlData>>&
      app_control_data_array() const;

  bool ReplyResult(
      const std::vector<std::unique_ptr<ApplicationControlData>>& data,
      const std::string& encoded_bundle) const;
  bool ReplyFailure(const std::string& encoded_bundle) const;

  void AddAppControlData(std::unique_ptr<ApplicationControlData> data);

  std::unique_ptr<picojson::value> ToJson() const;

  static std::unique_ptr<ApplicationControl>
      ApplicationControlFromService(service_h b);
  static std::unique_ptr<ApplicationControl>
      ApplicationControlFromJSON(const picojson::value& value);

 private:
  std::string operation_;
  std::string mime_;
  std::string uri_;
  std::string category_;
  std::vector<std::unique_ptr<ApplicationControlData>> app_control_data_array_;

  DISALLOW_COPY_AND_ASSIGN(ApplicationControl);
};

#endif  // APPLICATION_APPLICATION_CONTROL_H_

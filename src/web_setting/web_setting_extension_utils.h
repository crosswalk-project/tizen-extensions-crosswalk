// Copyright (c) 2014 Samsung Electronics Co., Ltd. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEB_SETTING_WEB_SETTING_EXTENSION_UTILS_H_
#define WEB_SETTING_WEB_SETTING_EXTENSION_UTILS_H_

#include <memory>

#include "common/picojson.h"
#include "tizen/tizen.h"

std::unique_ptr<picojson::value> CreateResultMessage() {
  picojson::object obj;
  obj["error"] = picojson::value();
  return std::unique_ptr<picojson::value>(new picojson::value(obj));
}

std::unique_ptr<picojson::value> CreateResultMessage(
    WebApiAPIErrors error) {
  picojson::object obj;
  obj["error"] = picojson::value(static_cast<double>(error));
  return std::unique_ptr<picojson::value>(new picojson::value(obj));
}

std::unique_ptr<picojson::value> CreateResultMessage(
    const picojson::object& data) {
  picojson::object obj;
  obj["data"] = picojson::value(data);
  return std::unique_ptr<picojson::value>(new picojson::value(obj));
}

std::unique_ptr<picojson::value> CreateResultMessage(
    const picojson::array& data) {
  picojson::object obj;
  obj["data"] = picojson::value(data);
  return std::unique_ptr<picojson::value>(new picojson::value(obj));
}

std::unique_ptr<picojson::value> CreateResultMessage(
    const picojson::value& data) {
  picojson::object obj;
  obj["data"] = data;
  return std::unique_ptr<picojson::value>(new picojson::value(obj));
}

#endif  // WEB_SETTING_WEB_SETTING_EXTENSION_UTILS_H_

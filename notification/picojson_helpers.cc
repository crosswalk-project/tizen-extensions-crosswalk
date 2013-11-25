// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "notification/picojson_helpers.h"

picojson::value ParseJSONMessage(const char* message) {
  picojson::value result;
  std::string err;
  picojson::parse(result, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cerr << "Notification: error parsing JSON message '"
              << err << "'\n";
  }
  return result;
}

picojson::value JSONValueFromInt(int value) {
  return picojson::value(static_cast<double>(value));
}

bool GetIntFromJSONValue(const picojson::value& v, int* result) {
  if (!result || !v.is<double>())
    return false;
  *result = v.get<double>();
  return true;
}

void GetStringFromJSONValue(const picojson::value& v, std::string* result) {
  if (!result)
    return;
  if (v.is<picojson::null>() || !v.is<std::string>()) {
    *result = "";
     return;
  }
  *result = v.to_str();
}

bool GetULongFromJSONValue(const picojson::value& v, uint64_t* result) {
  if (!result || !v.is<double>())
    return false;
  *result = v.get<double>();
  return true;
}

bool GetLongFromJSONValue(const picojson::value& v, int64_t* result) {
  if (!result || !v.is<double>())
    return false;
  *result = v.get<double>();
  return true;
}

void GetBoolFromJSONValue(const picojson::value& v, bool* result) {
  if (!result || !v.is<bool>())
    return;
  *result = v.evaluate_as_boolean();
}

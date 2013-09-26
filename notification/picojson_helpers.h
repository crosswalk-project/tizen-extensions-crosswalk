// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NOTIFICATION_PICOJSON_HELPERS_H_
#define NOTIFICATION_PICOJSON_HELPERS_H_

#include <string>
#include "common/picojson.h"

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

#endif  // NOTIFICATION_PICOJSON_HELPERS_H_

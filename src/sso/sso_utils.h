// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SSO_SSO_UTILS_H_
#define SSO_SSO_UTILS_H_

#include <string>
#include <vector>

#include "common/picojson.h"

class SSOUtils {
 public:
  SSOUtils();
  ~SSOUtils();

  static picojson::value FromStringToJSONValue(const char* string);
  static picojson::array FromStringArrayToJSONArray(
      const char* const* strings);
  static char** FromJSONArrayToStringArray(const picojson::array& array);
  static picojson::array FromStringVectorToJSONArray(
      const std::vector<std::string>& vector);
  static std::vector<std::string> FromJSONArrayToStringVector(
      const picojson::array& array);
  static std::vector<std::string> FromStringArrayToStringVector(
      const char* const* strings);
  static char** FromStringVectorToStringArray(
      const std::vector<std::string>& vector);
};

#endif  // SSO_SSO_UTILS_H_

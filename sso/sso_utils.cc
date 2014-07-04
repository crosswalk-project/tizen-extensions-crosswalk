// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sso/sso_utils.h"

#include <glib.h>

SSOUtils::SSOUtils() {
}

SSOUtils::~SSOUtils() {
}

picojson::value SSOUtils::FromStringToJSONValue(const char* string) {
  return string ? picojson::value(string) : picojson::value();
}

picojson::array SSOUtils::FromStringArrayToJSONArray(
    const char* const* strings) {
  picojson::array array;
  while (*strings) {
    array.push_back(picojson::value(*strings++));
  }
  return array;
}

char** SSOUtils::FromJSONArrayToStringArray(const picojson::array& array) {
  char** res = reinterpret_cast<char**>(g_malloc0(
      sizeof(char*) * (array.size() + 1)));
  std::vector<picojson::value>::const_iterator it;
  gint i = 0;
  for (it = array.begin(); it != array.end(); ++it) {
    picojson::value val = *it;
    res[i++] = g_strdup(val.to_str().c_str());
  }
  return res;
}

picojson::array SSOUtils::FromStringVectorToJSONArray(
    const std::vector<std::string>& vector) {
  picojson::array array;
  std::vector<std::string>::const_iterator it;
  for (it = vector.begin(); it != vector.end(); ++it) {
    array.push_back(picojson::value(*it));
  }
  return array;
}

std::vector<std::string> SSOUtils::FromJSONArrayToStringVector(
    const picojson::array& array) {
  std::vector<std::string> res;
  picojson::array::const_iterator it;
  for (it = array.begin(); it != array.end(); ++it) {
    picojson::value val = *it;
    res.push_back(val.to_str());
  }
  return res;
}

std::vector<std::string> SSOUtils::FromStringArrayToStringVector(
    const char* const* strings) {
  std::vector<std::string> vec;
  if (strings) {
    while (*strings)
      vec.push_back(*strings++);
  }
  return vec;
}

char** SSOUtils::FromStringVectorToStringArray(
    const std::vector<std::string>& vector) {
  char** res = reinterpret_cast<char**>(g_malloc0(
      sizeof(char*) * (vector.size() + 1)));
  gint i = 0;
  std::vector<std::string>::const_iterator it;
  for (it = vector.begin(); it != vector.end(); ++it) {
    res[i++] = g_strdup((*it).c_str());
  }
  return res;
}

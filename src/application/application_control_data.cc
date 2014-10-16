// Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "application/application_control_data.h"

ApplicationControlData::ApplicationControlData(
    const std::string& key,
    std::unique_ptr<std::vector<std::string>> values)
    : key_(key),
      values_(std::move(values)) {
}

const std::string& ApplicationControlData::key() const {
  return key_;
}

const std::vector<std::string>& ApplicationControlData::values() const {
  return *values_;
}

std::unique_ptr<picojson::value> ApplicationControlData::ToJson() const {
  picojson::object object;
  object["key"] = picojson::value(key_);
  picojson::array array;
  for (const auto& i : *values_) {
    array.push_back(picojson::value(i));
  }
  object["value"] = picojson::value(array);
  return std::unique_ptr<picojson::value>(new picojson::value(object));
}

std::unique_ptr<ApplicationControlData>
ApplicationControlData::ApplicationControlDataFromJSON(
    const picojson::value& value) {
  if (!value.is<picojson::object>())
    return nullptr;
  if (!value.contains("key"))
    return nullptr;
  const picojson::value& key_value = value.get("key");
  if (!key_value.is<std::string>())
    return nullptr;
  const std::string& key = key_value.get<std::string>();
  if (!value.contains("value"))
    return nullptr;
  const picojson::value& value_value = value.get("value");
  if (!value_value.is<picojson::array>())
    return nullptr;
  const picojson::array& array = value_value.get<picojson::array>();
  std::unique_ptr<std::vector<std::string>> values(
      new std::vector<std::string>());
  for (const auto& i : array) {
    if (!i.is<std::string>())
      return nullptr;
    values->push_back(i.get<std::string>());
  }
  return std::unique_ptr<ApplicationControlData>(
      new ApplicationControlData(key, std::move(values)));
}

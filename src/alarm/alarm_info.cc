// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "alarm/alarm_info.h"

#include <iostream>

#include "common/picojson.h"

std::string AlarmInfo::Serialize() {
  picojson::object obj;
  obj["id"] = picojson::value(static_cast<double>(id_));

  if (type_ == ABSOLUTE) {
    obj["type"] = picojson::value(std::string("absolute"));
    obj["date"] = picojson::value(static_cast<double>(date_));
    obj["period"] = picojson::value(static_cast<double>(period_));
    obj["weekFlag"] = picojson::value(static_cast<double>(weekflag_));
  } else if (type_ == RELATIVE) {
    obj["type"] = picojson::value(std::string("relative"));
    obj["delay"] = picojson::value(static_cast<double>(delay_));
    obj["period"] = picojson::value(static_cast<double>(period_));
  } else {
    // Should never come here.
    std::cerr << "Unknown alarm type " << type_ << std::endl;
  }

  picojson::value val(obj);
  return val.serialize();
}

bool AlarmInfo::Deserialize(const char* stream) {
  picojson::value obj;

  std::string err;
  picojson::parse(obj, stream, stream + strlen(stream), &err);
  if (!err.empty()) {
    std::cerr << "Failed to deserialize alarm object: " << stream << std::endl;
    return false;
  }

  id_ = static_cast<int>(obj.get("id").get<double>());
  std::string type = obj.get("type").to_str();

  if (type == "absolute") {
    type_ = ABSOLUTE;
    date_ = static_cast<int>(obj.get("date").get<double>());
    period_ = static_cast<int>(obj.get("period").get<double>());
    weekflag_ = static_cast<int>(obj.get("weekFlag").get<double>());
  } else if (type == "relative") {
    type_ = RELATIVE;
    delay_ = static_cast<int>(obj.get("delay").get<double>());
    period_ = static_cast<int>(obj.get("period").get<double>());
  } else {
    std::cerr << "Unknown alarm type " << type << std::endl;
    return false;
  }

  return true;
}

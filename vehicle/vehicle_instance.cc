// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "vehicle/vehicle_instance.h"

#include <abstractpropertytype.h>

#include <algorithm>
#include <string>

#include "vehicle/vehicle.h"

VehicleInstance::VehicleInstance(): vehicle_(new Vehicle(this)) {
  DebugOut::setDebugThreshhold(5);
}

void VehicleInstance::HandleMessage(const char* message) {
  DebugOut() << "VehicleInstance message received " << message << endl;

  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    return;
  }

  std::string method = v.get("method").to_str();

  if (method == "get") {
    std::string attribute = v.get("name").to_str();
    int callback_id = v.get("asyncCallId").get<double>();
    Zone::Type amb_zone = 0;
    if (v.contains("zone")) {
      picojson::value zone = v.get("zone");
      picojson::array zones = zone.get("value").get<picojson::array>();
      amb_zone = ZoneToAMBZone(zones);
    }

    std::transform(attribute.begin(), attribute.begin() + 1, attribute.begin(),
                   ::toupper);

    vehicle_->Get(attribute, amb_zone, callback_id);
  }
}

void VehicleInstance::HandleSyncMessage(const char* message) {
}

int VehicleInstance::ZoneToAMBZone(picojson::array zones) {
  Zone::Type amb_zone = 0;

  for (auto zone : zones) {
    std::string tempzone = zone.to_str();

    if (tempzone == "Front") {
      amb_zone |= Zone::Front;
    } else if (tempzone == "Middle") {
      amb_zone |= Zone::Middle;
    } else if (tempzone == "Right") {
      amb_zone |= Zone::Right;
    } else if (tempzone == "Left") {
      amb_zone |= Zone::Left;
    } else if (tempzone == "Rear") {
      amb_zone |= Zone::Rear;
    } else if (tempzone == "Center") {
      amb_zone |= Zone::Center;
    }
  }

  return amb_zone;
}

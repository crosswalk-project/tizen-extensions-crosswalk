// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "notification/notification_parameters.h"

#include "notification/picojson_helpers.h"

NotificationParameters ReadNotificationParameters(const picojson::value& v) {
  NotificationParameters params;
  GetStringFromJSONValue(v.get("statusType"), &params.status_type);

  GetStringFromJSONValue(v.get("title"), &params.title);
  GetStringFromJSONValue(v.get("content"), &params.content);

  GetStringFromJSONValue(v.get("iconPath"), &params.icon_path);

  GetStringFromJSONValue(v.get("soundPath"), &params.sound_path);
  GetBoolFromJSONValue(v.get("vibration"), &params.vibration);

  if (params.status_type == "PROGRESS") {
    GetStringFromJSONValue(v.get("progressType"), &params.progress_type);
    GetULongFromJSONValue(v.get("progressValue"), &params.progress_value);
  }

  GetStringFromJSONValue(v.get("subIconPath"), &params.sub_icon_path);

  GetStringFromJSONValue(v.get("ledColor"), &params.led_color);
  GetULongFromJSONValue(v.get("ledOnPeriod"), &params.led_on_period);
  GetULongFromJSONValue(v.get("ledOffPeriod"), &params.led_off_period);

  GetStringFromJSONValue(v.get("backgroundImagePath"),
                         &params.background_image_path);

  picojson::array t = v.get("thumbnails").get<picojson::array>();
  for (picojson::array::iterator iter = t.begin(); iter != t.end(); ++iter)
    params.thumbnails.push_back((*iter).to_str());

  return params;
}

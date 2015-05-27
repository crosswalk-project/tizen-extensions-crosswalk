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

  if (!GetLongFromJSONValue(v.get("number"), &params.number))
    params.number = 0;

  GetStringFromJSONValue(v.get("subIconPath"), &params.sub_icon_path);

  if (params.status_type == "SIMPLE") {
    picojson::array d = v.get("detailInfo").get<picojson::array>();
    for (unsigned i = 0; i < d.size(); i++) {
      GetStringFromJSONValue(d[i].get("mainText"),
                             &(params.detail_info[i].main_text));
      GetStringFromJSONValue(d[i].get("subText"),
                             &(params.detail_info[i].sub_text));
      params.detail_info[i].is_null = false;
    }
  }

  GetStringFromJSONValue(v.get("ledColor"), &params.led_color);
  if (!GetULongFromJSONValue(v.get("ledOnPeriod"), &params.led_on_period))
    params.led_on_period = 0;
  if (!GetULongFromJSONValue(v.get("ledOffPeriod"), &params.led_off_period))
    params.led_off_period = 0;

  GetStringFromJSONValue(v.get("backgroundImagePath"),
                         &params.background_image_path);

  picojson::array t = v.get("thumbnails").get<picojson::array>();
  for (picojson::array::iterator iter = t.begin(); iter != t.end(); ++iter)
    params.thumbnails.push_back((*iter).to_str());

  return params;
}

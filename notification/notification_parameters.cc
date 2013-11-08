// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "notification/notification_parameters.h"

#include "common/picojson.h"

NotificationParameters ReadNotificationParameters(const picojson::value& v) {
  NotificationParameters params;
  params.status_type = v.get("statusType").to_str();

  params.title = v.get("title").to_str();
  params.content = v.get("content").to_str();

  params.icon_path = v.get("iconPath").to_str();

  if (params.status_type == "PROGRESS") {
    params.progress_type = v.get("progressType").to_str();
    params.progress_value = v.get("progressValue").get<double>();
  }

  params.sub_icon_path = v.get("subIconPath").to_str();

  params.background_image_path = v.get("backgroundImagePath").to_str();

  picojson::array t = v.get("thumbnails").get<picojson::array>();
  for (picojson::array::iterator iter = t.begin(); iter != t.end(); ++iter)
    params.thumbnails.push_back((*iter).to_str());

  return params;
}

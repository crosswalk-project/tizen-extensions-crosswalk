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

  if (params.status_type == "PROGRESS") {
    params.progress_type = v.get("progressType").to_str();
    params.progress_value = v.get("progressValue").get<double>();
  }

  return params;
}

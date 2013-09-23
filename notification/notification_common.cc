// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "notification/notification_common.h"

#include "common/picojson.h"

NotificationParameters ReadNotificationParameters(const picojson::value& v) {
  NotificationParameters params;
  params.title = v.get("title").to_str();
  params.content = v.get("content").to_str();
  return params;
}

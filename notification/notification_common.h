// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NOTIFICATION_NOTIFICATION_COMMON_H_
#define NOTIFICATION_NOTIFICATION_COMMON_H_

#include <string>

namespace picojson {
class value;
}

struct NotificationParameters {
  std::string title;
  std::string content;
};

NotificationParameters ReadNotificationParameters(const picojson::value& v);

#endif  // NOTIFICATION_NOTIFICATION_COMMON_H_

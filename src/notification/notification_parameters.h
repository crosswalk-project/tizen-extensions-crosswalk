// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NOTIFICATION_NOTIFICATION_PARAMETERS_H_
#define NOTIFICATION_NOTIFICATION_PARAMETERS_H_

#include <string>
#include <vector>

namespace picojson {
class value;
}

struct DetailInfo {
  std::string main_text;
  std::string sub_text;
  bool is_null;
  DetailInfo() : is_null(true) {}
};

struct NotificationParameters {
  std::string status_type;

  std::string title;
  std::string content;

  std::string icon_path;

  std::string sound_path;
  bool vibration;

  std::string progress_type;
  uint64_t progress_value;

  int64_t number;

  std::string sub_icon_path;

  DetailInfo detail_info[2];

  std::string led_color;
  uint64_t led_on_period;
  uint64_t led_off_period;

  std::string background_image_path;
  std::vector<std::string> thumbnails;
};

NotificationParameters ReadNotificationParameters(const picojson::value& v);

#endif  // NOTIFICATION_NOTIFICATION_PARAMETERS_H_

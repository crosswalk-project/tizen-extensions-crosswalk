// Copyright (c) 2014 Samsung Electronics Co., Ltd. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEB_SETTING_WEB_SETTING_H_
#define WEB_SETTING_WEB_SETTING_H_

#include <gio/gio.h>
#include <memory>
#include <string>

#include "common/picojson.h"

class WebSetting {
 public:
  explicit WebSetting(const std::string& app_id);
  ~WebSetting();

  std::unique_ptr<picojson::value> RemoveAllCookies();
  std::unique_ptr<picojson::value> SetUserAgentString(
      const std::string& user_agent);

 private:
  std::string app_id_;
  GDBusProxy* running_app_proxy_;
};

#endif  // WEB_SETTING_WEB_SETTING_H_

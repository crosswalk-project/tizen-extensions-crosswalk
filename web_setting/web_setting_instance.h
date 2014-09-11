// Copyright (c) 2014 Samsung Electronics Co., Ltd. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEB_SETTING_WEB_SETTING_INSTANCE_H_
#define WEB_SETTING_WEB_SETTING_INSTANCE_H_

#include "common/extension.h"
#include "common/picojson.h"
#include "tizen/tizen.h"

#include "web_setting/web_setting_extension.h"

class WebSettingInstance : public common::Instance {
 public:
  explicit WebSettingInstance(WebSettingExtension* extension);
  virtual ~WebSettingInstance();

 private:
  void HandleMessage(const char* message);

  void HandleSetUserAgentString(const picojson::value& msg);
  void HandleRemoveAllCookies(const picojson::value& msg);

  void ReturnMessageAsync(double callback_id, picojson::value& value);

  WebSettingExtension* extension_;
};

#endif  // WEB_SETTING_WEB_SETTING_INSTANCE_H_

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "notification/notification_context.h"
#include "common/picojson.h"

CXWalkExtension* xwalk_extension_init(int32_t api_version) {
  NotificationContext::PlatformInitialize();
  return ExtensionAdapter<NotificationContext>::Initialize();
}

const char NotificationContext::name[] = "tizen.notification";

// This will be generated from notification_api.js.
extern const char kSource_notification_api[];

const char* NotificationContext::GetJavaScript() {
  return kSource_notification_api;
}

void NotificationContext::HandleMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "NotificationPost")
    HandlePost(v);
  else if (cmd == "NotificationRemove")
    HandleRemove(v);
}

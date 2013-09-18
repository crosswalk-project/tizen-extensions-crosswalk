// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "notification/notification_context.h"

#include <iostream>
#include "common/picojson.h"

namespace {

// We have one NotificationManager per extension (so, per process). This is
// like this because we can have only one callback function to track the
// eventual removal of notifications.
NotificationManager* g_notification_manager = NULL;

void NotificationSetText(notification_h notification,
                         notification_text_type_e type,
                         const std::string& text) {
  notification_set_text(
      notification, type, text.c_str(), NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
}

}  // namespace

// static
void NotificationContext::PlatformInitialize() {
  // FIXME(cmarcelo): Handle shutdown.
  g_notification_manager = new NotificationManager();
}

NotificationContext::NotificationContext(ContextAPI* api)
    : api_(api) {}

NotificationContext::~NotificationContext() {
  g_notification_manager->DetachClient(this);
  delete api_;
}

void NotificationContext::HandlePost(const picojson::value& msg) {
  notification_h notification = g_notification_manager->CreateNotification();
  NotificationSetText(notification, NOTIFICATION_TEXT_TYPE_TITLE,
                      msg.get("title").to_str());
  NotificationSetText(notification, NOTIFICATION_TEXT_TYPE_CONTENT,
                      msg.get("content").to_str());

  std::string id = msg.get("id").to_str();
  if (!g_notification_manager->PostNotification(id, notification, this)) {
    std::cerr << "tizen.notification error: "
              << " couldn't post notification with id '" << id << "'";
    return;
  }
}

void NotificationContext::HandleRemove(const picojson::value& msg) {
  std::string id = msg.get("id").to_str();
  if (!g_notification_manager->RemoveNotification(id)) {
    std::cerr << "tizen.notification error: "
              << "couldn't remove notification with id '" << id << "'\n";
  }
}

void NotificationContext::OnNotificationRemoved(const std::string& id) {
  picojson::value::object o;
  o["cmd"] = picojson::value("NotificationRemoved");
  o["id"] = picojson::value(id);
  picojson::value v(o);
  api_->PostMessage(v.serialize().c_str());
}

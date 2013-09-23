// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "notification/notification_instance_mobile.h"

#include "common/picojson.h"

namespace {

void NotificationSetText(notification_h notification,
                         notification_text_type_e type,
                         const std::string& text) {
  notification_set_text(
      notification, type, text.c_str(), NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
}

}  // namespace

NotificationInstanceMobile::NotificationInstanceMobile(
    NotificationManager* manager)
    : manager_(manager) {}

NotificationInstanceMobile::~NotificationInstanceMobile() {
  manager_->DetachClient(this);
}

void NotificationInstanceMobile::HandleMessage(const char* message) {
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

void NotificationInstanceMobile::HandlePost(const picojson::value& msg) {
  notification_h notification = manager_->CreateNotification();
  NotificationSetText(notification, NOTIFICATION_TEXT_TYPE_TITLE,
                      msg.get("title").to_str());
  NotificationSetText(notification, NOTIFICATION_TEXT_TYPE_CONTENT,
                      msg.get("content").to_str());

  std::string id = msg.get("id").to_str();
  if (!manager_->PostNotification(id, notification, this)) {
    std::cerr << "tizen.notification error: "
              << " couldn't post notification with id '" << id << "'";
    return;
  }
}

void NotificationInstanceMobile::HandleRemove(const picojson::value& msg) {
  std::string id = msg.get("id").to_str();
  if (!manager_->RemoveNotification(id)) {
    std::cerr << "tizen.notification error: "
              << "couldn't remove notification with id '" << id << "'\n";
  }
}

void NotificationInstanceMobile::OnNotificationRemoved(const std::string& id) {
  picojson::value::object o;
  o["cmd"] = picojson::value("NotificationRemoved");
  o["id"] = picojson::value(id);
  picojson::value v(o);
  PostMessage(v.serialize().c_str());
}


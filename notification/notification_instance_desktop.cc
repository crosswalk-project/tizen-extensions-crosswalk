// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "notification/notification_instance_desktop.h"

#include "common/picojson.h"

NotificationInstanceDesktop::NotificationInstanceDesktop() {}

NotificationInstanceDesktop::~NotificationInstanceDesktop() {}

void NotificationInstanceDesktop::HandleMessage(const char* message) {
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

void NotificationInstanceDesktop::HandlePost(const picojson::value& msg) {
  NotifyNotification* notification =
      notify_notification_new(msg.get("title").to_str().c_str(),
                              msg.get("content").to_str().c_str(),
                              NULL);
  notify_notification_set_timeout(notification, NOTIFY_EXPIRES_NEVER);
  g_signal_connect(notification, "closed",
                   G_CALLBACK(OnNotificationClosedThunk), this);
  notify_notification_show(notification, NULL);

  std::string id = msg.get("id").to_str();
  notifications_[id] = notification;
}

void NotificationInstanceDesktop::HandleRemove(const picojson::value& msg) {
  std::string id = msg.get("id").to_str();
  NotificationsMap::iterator it = notifications_.find(id);
  if (it == notifications_.end())
    return;
  notify_notification_close(it->second, NULL);
}

std::string NotificationInstanceDesktop::IdFromNotification(
    NotifyNotification* notification) {
  NotificationsMap::iterator it;
  for (it = notifications_.begin(); it != notifications_.end(); ++it) {
    if (it->second == notification)
      return it->first;
  }
  return std::string();
}

void NotificationInstanceDesktop::OnNotificationClosedThunk(
    NotifyNotification* notification, gpointer data) {
  NotificationInstanceDesktop* handler =
      reinterpret_cast<NotificationInstanceDesktop*>(data);
  handler->OnNotificationClosed(notification);
}

void NotificationInstanceDesktop::OnNotificationClosed(
    NotifyNotification* notification) {
  const std::string id = IdFromNotification(notification);
  if (id.empty())
    return;
  picojson::value::object o;
  o["cmd"] = picojson::value("NotificationRemoved");
  o["id"] = picojson::value(id);
  picojson::value v(o);
  PostMessage(v.serialize().c_str());

  g_object_unref(notification);
  notifications_.erase(id);
}

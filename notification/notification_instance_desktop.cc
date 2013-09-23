// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "notification/notification_instance_desktop.h"

#include <iostream>
#include "common/picojson.h"
#include "notification/picojson_helpers.h"

NotificationInstanceDesktop::NotificationInstanceDesktop() {}

NotificationInstanceDesktop::~NotificationInstanceDesktop() {}

namespace {

int GetNextUniqueID() {
  static int next_id = 1;
  return next_id++;
}

}

void NotificationInstanceDesktop::HandleMessage(const char* message) {
  picojson::value v = ParseJSONMessage(message);
  std::string cmd = v.get("cmd").to_str();
  if (cmd == "NotificationRemove")
    HandleRemove(v);
  else
    std::cerr << "Notification: received invalid command '" << cmd << "'\n";
}

void NotificationInstanceDesktop::HandleSyncMessage(const char* message) {
  picojson::value v = ParseJSONMessage(message);
  std::string cmd = v.get("cmd").to_str();
  if (cmd == "NotificationPost")
    HandlePost(v);
  else
    std::cerr << "Notification: received invalid command '" << cmd << "'\n";
}

void NotificationInstanceDesktop::HandlePost(const picojson::value& msg) {
  NotifyNotification* notification =
      notify_notification_new(msg.get("title").to_str().c_str(),
                              msg.get("content").to_str().c_str(),
                              NULL);
  notify_notification_set_timeout(notification, NOTIFY_EXPIRES_NEVER);
  gulong handler = g_signal_connect(
      notification, "closed", G_CALLBACK(OnNotificationClosedThunk), this);
  if (!notify_notification_show(notification, NULL)) {
    g_signal_handler_disconnect(notification, handler);
    SendSyncReply(picojson::value().serialize().c_str());
    return;
  }

  int id = GetNextUniqueID();
  notifications_[id] = notification;
  SendSyncReply(JSONValueFromInt(id).serialize().c_str());
}

void NotificationInstanceDesktop::HandleRemove(const picojson::value& msg) {
  int id = msg.get("id").get<double>();
  NotificationsMap::iterator it = notifications_.find(id);
  if (it == notifications_.end())
    return;
  notify_notification_close(it->second, NULL);
}

int NotificationInstanceDesktop::IdFromNotification(
    NotifyNotification* notification) {
  NotificationsMap::iterator it;
  for (it = notifications_.begin(); it != notifications_.end(); ++it) {
    if (it->second == notification)
      return it->first;
  }
  return 0;
}

void NotificationInstanceDesktop::OnNotificationClosedThunk(
    NotifyNotification* notification, gpointer data) {
  NotificationInstanceDesktop* handler =
      reinterpret_cast<NotificationInstanceDesktop*>(data);
  handler->OnNotificationClosed(notification);
}

void NotificationInstanceDesktop::OnNotificationClosed(
    NotifyNotification* notification) {
  const int id = IdFromNotification(notification);
  if (!id)
    return;
  picojson::value::object o;
  o["cmd"] = picojson::value("NotificationRemoved");
  o["id"] = JSONValueFromInt(id);
  picojson::value v(o);
  PostMessage(v.serialize().c_str());

  g_object_unref(notification);
  notifications_.erase(id);
}

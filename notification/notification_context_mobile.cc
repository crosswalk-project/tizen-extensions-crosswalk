// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "notification/notification_context.h"

#include "common/picojson.h"
#include <iostream>
#include <map>
#include <notification.h>

namespace {

void error(const char* msg) {
  std::cerr << "tizen.notification extension error: " << msg << "\n";
}

}  // namespace

// Uses Tizen notification library to post notifications and keep track on
// updates made to them.
class NotificationManager {
 public:
  NotificationManager();
  ~NotificationManager();

  notification_h CreateNotification();
  int PostNotification(notification_h notification, NotificationClient* client);
  void RemoveNotification(int priv_id);

  void DetachClient(NotificationClient* client);

 private:
  // Trampoline to get to the instance function below.
  static void OnDetailedChanged(void* data, notification_type_e type,
                                notification_op* op_list, int num_op) {
    NotificationManager* self = reinterpret_cast<NotificationManager*>(data);
    self->OnDetailedChanged(type, op_list, num_op);
  }

  void OnDetailedChanged(
      notification_type_e type, notification_op* op_list, int num_op);

  typedef std::map<int, NotificationClient*> PrivIDToClientMap;
  PrivIDToClientMap priv_ids_;
};

NotificationManager::NotificationManager() {
  notification_register_detailed_changed_cb(
      OnDetailedChanged, reinterpret_cast<void*>(this));
}

NotificationManager::~NotificationManager() {}

notification_h NotificationManager::CreateNotification() {
  return notification_new(NOTIFICATION_TYPE_NOTI,
                          NOTIFICATION_GROUP_ID_NONE,
                          NOTIFICATION_PRIV_ID_NONE);
}

int NotificationManager::PostNotification(notification_h notification,
                                          NotificationClient* client) {
  int priv_id;
  notification_error_e err = notification_insert(notification, &priv_id);
  if (err != NOTIFICATION_ERROR_NONE) {
    error("Can't insert notification");
    return NOTIFICATION_PRIV_ID_NONE;
  }
  return priv_id;
}

void NotificationManager::RemoveNotification(int priv_id) {
  PrivIDToClientMap::iterator it = priv_ids_.find(priv_id);
  if (it == priv_ids_.end()) {
    error("Trying to remove invalid notification");
    return;
  }

  // We don't erase this priv_id from the map here, but when the
  // OnDetailedChanged is called later.
  notification_error_e err = notification_delete_by_priv_id(
      NULL, NOTIFICATION_TYPE_NOTI, it->first);
  if (err != NOTIFICATION_ERROR_NONE) {
    error("Can't remove notification");
    return;
  }
}

void NotificationManager::OnDetailedChanged(
    notification_type_e type, notification_op* op_list, int num_op) {
  // This function gets warned about every notification event for every
  // application. This code filters so we just look into removals of priv_ids
  // handled by this manager.
  for (int i = 0; i < num_op; i++) {
    notification_op* operation = &op_list[i];
    if (operation->type != NOTIFICATION_OP_DELETE)
      continue;

    PrivIDToClientMap::iterator it = priv_ids_.find(operation->priv_id);
    if (it == priv_ids_.end())
      continue;

    it->second->OnNotificationRemoved(it->first);
    priv_ids_.erase(it);
  }
}

namespace {

NotificationManager* g_notification_manager = NULL;

}  // namespace

// static
void NotificationContext::PlatformInitialize() {
  // FIXME(cmarcelo): Handle shutdown.
  g_notification_manager = new NotificationManager();
}

NotificationContext::NotificationContext(ContextAPI* api)
    : api_(api) {}

NotificationContext::~NotificationContext() {}

void NotificationContext::HandlePost(const picojson::value& msg) {

}

void NotificationContext::HandleRemove(const picojson::value& msg) {

}

void NotificationContext::OnNotificationRemoved(int priv_id) {

}

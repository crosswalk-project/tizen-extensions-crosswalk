// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "notification/mobile/notification_manager.h"

#include <algorithm>

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

bool NotificationManager::PostNotification(const std::string& id,
                                           notification_h notification,
                                           NotificationClient* client) {
  int priv_id;
  notification_error_e err = notification_insert(notification, &priv_id);
  if (err != NOTIFICATION_ERROR_NONE)
    return false;

  NotificationEntry entry;
  entry.priv_id = priv_id;
  entry.handle = notification;
  entry.client = client;

  id_map_[id] = entry;
  return true;
}

bool NotificationManager::RemoveNotification(const std::string& id) {
  IDMap::iterator it = id_map_.find(id);
  if (it == id_map_.end())
    return false;

  // We don't erase the entry from the map here, but when the OnDetailedChanged
  // is called later.
  const NotificationEntry& entry = it->second;
  notification_error_e err = notification_delete_by_priv_id(
      NULL, NOTIFICATION_TYPE_NOTI, entry.priv_id);
  return (err == NOTIFICATION_ERROR_NONE);
}

void NotificationManager::DetachClient(NotificationClient* client) {
  IDMap::iterator it = id_map_.begin();
  while (it != id_map_.end()) {
    IDMap::iterator current = it++;
    NotificationEntry entry = current->second;
    if (entry.client == client) {
      id_map_.erase(current);
      notification_free(entry.handle);
    }
  }
}

namespace {

template <class T>
struct MatchPrivID {
  explicit MatchPrivID(int priv_id) : priv_id(priv_id) {}
  bool operator()(const typename T::value_type& value) {
    return value.second.priv_id == priv_id;
  }
  int priv_id;
};

}  // namespace

void NotificationManager::OnDetailedChanged(
    notification_type_e type, notification_op* op_list, int num_op) {
  // This function gets warned about every notification event for every
  // application. This code filters so we just look into removals of priv_ids
  // handled by this manager.
  for (int i = 0; i < num_op; i++) {
    notification_op* operation = &op_list[i];
    if (operation->type != NOTIFICATION_OP_DELETE)
      continue;

    IDMap::iterator it = std::find_if(id_map_.begin(), id_map_.end(),
                                      MatchPrivID<IDMap>(operation->priv_id));
    if (it == id_map_.end())
      continue;

    NotificationEntry entry = it->second;
    entry.client->OnNotificationRemoved(it->first);
    id_map_.erase(it);
    notification_free(entry.handle);
  }
}

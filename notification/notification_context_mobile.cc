// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "notification/notification_context.h"

#include "common/picojson.h"
#include <iostream>
#include <map>
#include <notification.h>
#include <pthread.h>

// Uses Tizen notification library to post notifications and keep track on
// updates made to them. Since clients can be running in different threads,
// public functions of the manager are thread safe.
class NotificationManager {
 public:
  NotificationManager();
  ~NotificationManager();

  // Create a notification_h handle that should be filled using the Tizen
  // notification library functions. We control creation so we can pre-set
  // certain parameters if necessary. This function is thread-safe.
  notification_h CreateNotification();

  // Post a notification created with the function above. The passed client will
  // be informed when the notification was destroyed. Return value is false if
  // the notification couldn't be posted. This function is thread-safe.
  //
  // Ownership of notification_h is taken by the NotificationManager.
  bool PostNotification(const std::string& id, notification_h notification,
                        NotificationClient* client);

  // Asks for a Notification to be removed, should be called with the identifier
  // received from PostNotification. If returns false, an error happened; if
  // true the removal was dispatched. Later the function OnNotificationRemoved()
  // from the client associated with the id will be called. This function is
  // thread-safe.
  bool RemoveNotification(const std::string& id);

  // Called when a Client is being destroyed, so we stop watching its
  // notifications.
  void DetachClient(NotificationClient* client);

 private:
  // Trampoline to get to the instance function below.
  static void OnDetailedChanged(void* data, notification_type_e type,
                                notification_op* op_list, int num_op) {
    NotificationManager* self = reinterpret_cast<NotificationManager*>(data);
    self->OnDetailedChanged(type, op_list, num_op);
  }

  // Callback from Tizen notification library to watch for changes in
  // notifications. Used to track removals (either caused by the API or by
  // user interaction).
  void OnDetailedChanged(
      notification_type_e type, notification_op* op_list, int num_op);

  struct NotificationEntry {
    int priv_id;
    notification_h handle;
    NotificationClient* client;
  };

  typedef std::map<std::string, NotificationEntry> IDMap;
  IDMap::iterator FindByPrivID(int priv_id) {
    IDMap::iterator it = id_map_.begin();
    while (it != id_map_.end()) {
      if (it->second.priv_id == priv_id)
        break;
    }
    return it;
  }

  IDMap id_map_;

  // Utility to lock this object in a certain scope.
  struct Locker {
    Locker(pthread_mutex_t* m) : m_(m) { pthread_mutex_lock(m_); }
    ~Locker() { pthread_mutex_unlock(m_); }
   private:
    pthread_mutex_t* m_;
  };

  pthread_mutex_t mutex_;
};

NotificationManager::NotificationManager() {
  pthread_mutex_init(&mutex_, NULL);
  notification_register_detailed_changed_cb(
      OnDetailedChanged, reinterpret_cast<void*>(this));
}

NotificationManager::~NotificationManager() {
  // Extension should outlive all its contexts, so when it is shutdown, all the
  // contexts were destroyed, so no contention here.
  pthread_mutex_destroy(&mutex_);
}

notification_h NotificationManager::CreateNotification() {
  Locker l(&mutex_);
  return notification_new(NOTIFICATION_TYPE_NOTI,
                          NOTIFICATION_GROUP_ID_NONE,
                          NOTIFICATION_PRIV_ID_NONE);
}

bool NotificationManager::PostNotification(const std::string& id,
                                           notification_h notification,
                                           NotificationClient* client) {
  Locker l(&mutex_);
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
  Locker l(&mutex_);
  IDMap::iterator it = id_map_.find(id);
  if (it == id_map_.end())
    return false;

  // We don't erase the entry from the map here, but when the OnDetailedChanged
  // is called later.
  const NotificationEntry& entry = it->second;
  notification_error_e err = notification_delete_by_priv_id(
      NULL, NOTIFICATION_TYPE_NOTI, entry.priv_id);
  if (err != NOTIFICATION_ERROR_NONE)
    return false;

  return true;
}

void NotificationManager::DetachClient(NotificationClient* client) {
  Locker l(&mutex_);
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

void NotificationManager::OnDetailedChanged(
    notification_type_e type, notification_op* op_list, int num_op) {
  Locker l(&mutex_);
  // This function gets warned about every notification event for every
  // application. This code filters so we just look into removals of priv_ids
  // handled by this manager.
  for (int i = 0; i < num_op; i++) {
    notification_op* operation = &op_list[i];
    if (operation->type != NOTIFICATION_OP_DELETE)
      continue;

    IDMap::iterator it = FindByPrivID(operation->priv_id);
    if (it == id_map_.end())
      continue;

    NotificationEntry entry = it->second;
    entry.client->OnNotificationRemoved(it->first);
    id_map_.erase(it);
    notification_free(entry.handle);
  }
}

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

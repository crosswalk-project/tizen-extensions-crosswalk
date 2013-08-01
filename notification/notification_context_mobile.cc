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
  // be informed when the notification was destroyed. Return value is either a
  // priv_id identifying it or NOTIFICATION_PRIV_ID_NONE in case of error.
  //
  // Ownership of notification_h is taken by the NotificationManager.
  int PostNotification(notification_h notification, NotificationClient* client);

  // Asks for a Notification to be removed, should be called with the identifier
  // received from PostNotification. If returns false, an error happened; if
  // true the removal was dispatched. Later the function OnNotificationRemoved()
  // from the client associated with priv_id will be called.
  bool RemoveNotification(int priv_id);

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
    notification_h handle;
    NotificationClient* client;
  };
  typedef std::map<int, NotificationEntry> PrivIDMap;
  PrivIDMap priv_ids_;

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
  pthread_mutex_destroy(&mutex_);
}

notification_h NotificationManager::CreateNotification() {
  Locker l(&mutex_);
  return notification_new(NOTIFICATION_TYPE_NOTI,
                          NOTIFICATION_GROUP_ID_NONE,
                          NOTIFICATION_PRIV_ID_NONE);
}

int NotificationManager::PostNotification(notification_h notification,
                                          NotificationClient* client) {
  Locker l(&mutex_);
  int priv_id;
  notification_error_e err = notification_insert(notification, &priv_id);
  if (err != NOTIFICATION_ERROR_NONE)
    return NOTIFICATION_PRIV_ID_NONE;

  NotificationEntry entry;
  entry.handle = notification;
  entry.client = client;

  priv_ids_[priv_id] = entry;
  return priv_id;
}

bool NotificationManager::RemoveNotification(int priv_id) {
  Locker l(&mutex_);
  PrivIDMap::iterator it = priv_ids_.find(priv_id);
  if (it == priv_ids_.end())
    return false;

  // We don't erase this priv_id from the map here, but when the
  // OnDetailedChanged is called later.
  notification_error_e err = notification_delete_by_priv_id(
      NULL, NOTIFICATION_TYPE_NOTI, it->first);
  if (err != NOTIFICATION_ERROR_NONE)
    return false;

  return true;
}

void NotificationManager::DetachClient(NotificationClient* client) {
  Locker l(&mutex_);
  PrivIDMap::iterator it = priv_ids_.begin();
  while (it != priv_ids_.end()) {
    PrivIDMap::iterator current = it++;
    NotificationEntry entry = current->second;
    if (entry.client == client) {
      priv_ids_.erase(current);
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

    PrivIDMap::iterator it = priv_ids_.find(operation->priv_id);
    if (it == priv_ids_.end())
      continue;

    NotificationEntry entry = it->second;
    entry.client->OnNotificationRemoved(it->first);
    priv_ids_.erase(it);
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
  int priv_id = g_notification_manager->PostNotification(notification, this);
  if (priv_id == NOTIFICATION_PRIV_ID_NONE) {
    std::cerr << "tizen.notification error: "
              << " couldn't post notification with id '" << id << "'";
    return;
  }

  notifications_[id] = priv_id;
}

void NotificationContext::HandleRemove(const picojson::value& msg) {
  std::string id = msg.get("id").to_str();
  NotificationsMap::iterator it = notifications_.find(id);
  if (it == notifications_.end()) {
    std::cerr << "tizen.notification error: "
              << "couldn't find notification with id '"
              << id << "' to remove\n";
    return;
  }

  int priv_id = it->second;
  if (!g_notification_manager->RemoveNotification(priv_id)) {
    std::cerr << "tizen.notification error: "
              << "couldn't remove notification with id '" << id << "'\n";
  }
}

void NotificationContext::OnNotificationRemoved(int priv_id) {
  NotificationsMap::iterator it = notifications_.begin();
  while (it != notifications_.end()) {
    if (it->second == priv_id)
      break;
    ++it;
  }

  if (it == notifications_.end()) {
    std::cerr << "tizen.notification error: internal error when removing "
              << "notification with priv_id=" << priv_id << "\n";
    return;
  }

  const std::string id = it->first;
  picojson::value::object o;
  o["cmd"] = picojson::value("NotificationRemoved");
  o["id"] = picojson::value(id);
  picojson::value v(o);
  api_->PostMessage(v.serialize().c_str());

  // FIXME(cmarcelo): race condition.
  notifications_.erase(it);
}

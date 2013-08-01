// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NOTIFICATION_NOTIFICATION_MOBILE_NOTIFICATION_MANAGER_H_
#define NOTIFICATION_NOTIFICATION_MOBILE_NOTIFICATION_MANAGER_H_

#include <map>
#include <notification.h>
#include <pthread.h>
#include <string>

class NotificationClient {
 public:
  virtual void OnNotificationRemoved(const std::string& id) = 0;
 protected:
  virtual ~NotificationClient() {}
};

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
  // notifications. This function is thread-safe.
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

#endif  // NOTIFICATION_NOTIFICATION_MOBILE_NOTIFICATION_MANAGER_H_

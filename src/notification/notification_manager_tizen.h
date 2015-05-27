// Copyright (c 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NOTIFICATION_NOTIFICATION_MANAGER_TIZEN_H_
#define NOTIFICATION_NOTIFICATION_MANAGER_TIZEN_H_

#include <notification.h>
#include <map>

class NotificationClient {
 public:
  virtual void OnNotificationRemoved(int id) = 0;
 protected:
  virtual ~NotificationClient() {}
};

// Uses Tizen notification library to post notifications and keep track on
// updates made to them.
class NotificationManager {
 public:
  NotificationManager();
  ~NotificationManager();

  // Create a notification_h handle that should be filled using the Tizen
  // notification library functions. We control creation so we can pre-set
  // certain parameters if necessary.
  notification_h CreateNotification(notification_type_e type);

  // Post a notification created with the function above. The passed client will
  // be informed when the notification was destroyed. Return value is 0 if the
  // notification couldn't be posted or the id in case of success.
  //
  // Ownership of notification_h is taken by the NotificationManager.
  int PostNotification(notification_h notification, NotificationClient* client);

  // Asks for a Notification to be removed, should be called with the identifier
  // received from PostNotification. If returns false, an error happened; if
  // true the removal was dispatched. Later the function OnNotificationRemoved()
  // from the client associated with the id will be called.
  bool RemoveNotification(int id);

  notification_h GetNotification(int id);

  bool UpdateNotification(notification_h notification);

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

  typedef std::map<int, NotificationEntry> IDMap;
  IDMap id_map_;
};

#endif  // NOTIFICATION_NOTIFICATION_MANAGER_TIZEN_H_

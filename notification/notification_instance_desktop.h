// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NOTIFICATION_NOTIFICATION_INSTANCE_DESKTOP_H_
#define NOTIFICATION_NOTIFICATION_INSTANCE_DESKTOP_H_

#if defined(GENERIC_DESKTOP)
#include <libnotify/notify.h>
#endif

#include <string>
#include <map>
#include "common/extension.h"

namespace picojson {
class value;
}

class NotificationInstanceDesktop : public common::Instance {
 public:
  NotificationInstanceDesktop();
  virtual ~NotificationInstanceDesktop();

 private:
  // common::Instance implementation.
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* msg);

  void HandlePost(const picojson::value& msg);
  void HandleRemove(const picojson::value& msg);
  void HandleUpdate(const picojson::value& msg);

  int IdFromNotification(NotifyNotification* notification);
  static void OnNotificationClosedThunk(NotifyNotification* notification,
                                        gpointer data);
  void OnNotificationClosed(NotifyNotification* notification);

  typedef std::map<int, NotifyNotification*> NotificationsMap;
  NotificationsMap notifications_;
};

#endif  // NOTIFICATION_NOTIFICATION_INSTANCE_DESKTOP_H_

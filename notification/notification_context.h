// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NOTIFICATION_NOTIFICATION_CONTEXT_H_
#define NOTIFICATION_NOTIFICATION_CONTEXT_H_

#if defined(GENERIC_DESKTOP)
#include <libnotify/notify.h>
#endif

#include <map>
#include <string>
#include "common/extension_adapter.h"

#if defined(TIZEN_MOBILE)
#include "notification/mobile/notification_manager.h"
#endif

namespace picojson {
class value;
}

class NotificationContext
#if defined(TIZEN_MOBILE)
    : public NotificationClient
#endif
{
 public:
  explicit NotificationContext(ContextAPI* api);
  ~NotificationContext();

  static void PlatformInitialize();

  // ExtensionAdapter implementation.
  static const char name[];
  static const char* GetJavaScript();
  void HandleMessage(const char* message);
  void HandleSyncMessage(const char* message) {}

 private:
  void HandlePost(const picojson::value& msg);
  void HandleRemove(const picojson::value& msg);

  ContextAPI* api_;

#if defined(GENERIC_DESKTOP)
  std::string IdFromNotification(NotifyNotification* notification);
  static void OnNotificationClosedThunk(NotifyNotification* notification,
                                        gpointer data);
  void OnNotificationClosed(NotifyNotification* notification);

  typedef std::map<std::string, NotifyNotification*> NotificationsMap;
  NotificationsMap notifications_;
#endif

#if defined(TIZEN_MOBILE)
  // NotificationClient implementation.
  virtual void OnNotificationRemoved(const std::string& id);

  typedef std::map<std::string, int> NotificationsMap;
  NotificationsMap notifications_;
#endif
};

#endif  // NOTIFICATION_NOTIFICATION_CONTEXT_H_

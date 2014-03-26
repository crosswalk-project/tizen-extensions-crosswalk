// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NOTIFICATION_NOTIFICATION_INSTANCE_TIZEN_H_
#define NOTIFICATION_NOTIFICATION_INSTANCE_TIZEN_H_

#include <string>
#include <map>
#include "common/extension.h"
#include "notification/notification_manager_tizen.h"

namespace picojson {
class value;
}

class NotificationInstanceMobile
    : public common::Instance,
      public NotificationClient {
 public:
  explicit NotificationInstanceMobile(NotificationManager* manager);
  virtual ~NotificationInstanceMobile();

 private:
  // common::Instance implementation.
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* msg);

  void HandlePost(const picojson::value& msg);
  void HandleRemove(const picojson::value& msg);
  void HandleUpdate(const picojson::value& msg);

  // NotificationClient implementation.
  virtual void OnNotificationRemoved(int id);

  NotificationManager* manager_;
};

#endif  // NOTIFICATION_NOTIFICATION_INSTANCE_TIZEN_H_

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NOTIFICATION_NOTIFICATION_INSTANCE_MOBILE_H_
#define NOTIFICATION_NOTIFICATION_INSTANCE_MOBILE_H_

#include <string>
#include <map>
#include "common/extension.h"
#include "notification/mobile/notification_manager.h"

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

  void HandlePost(const picojson::value& msg);
  void HandleRemove(const picojson::value& msg);

  // NotificationClient implementation.
  virtual void OnNotificationRemoved(const std::string& id);

  NotificationManager* manager_;
};

#endif  // NOTIFICATION_NOTIFICATION_INSTANCE_MOBILE_H_

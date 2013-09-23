// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NOTIFICATION_NOTIFICATION_EXTENSION_H_
#define NOTIFICATION_NOTIFICATION_EXTENSION_H_

#include "common/extension.h"

#if defined(TIZEN_MOBILE)
#include "notification/mobile/notification_manager.h"
#endif

class NotificationExtension : public common::Extension {
 public:
  NotificationExtension();
  virtual ~NotificationExtension();

 private:
  // common::Extension implementation.
  virtual common::Instance* CreateInstance();

#if defined(TIZEN_MOBILE)
  NotificationManager manager_;
#endif
};

#endif  // NOTIFICATION_NOTIFICATION_EXTENSION_H_

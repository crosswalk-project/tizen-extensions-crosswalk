// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "notification/notification_extension.h"

#if defined(GENERIC_DESKTOP)
#include <libnotify/notify.h>
#include "notification/notification_instance_desktop.h"
#elif defined(TIZEN_MOBILE)
#include "notification/notification_instance_mobile.h"
#endif

common::Extension* CreateExtension() {
  return new NotificationExtension;
}

// This will be generated from notification_api.js.
extern const char kSource_notification_api[];

NotificationExtension::NotificationExtension() {
#if defined(GENERIC_DESKTOP)
  notify_init("Crosswalk");
#endif
  SetExtensionName("tizen.notification");
  SetJavaScriptAPI(kSource_notification_api);
}

NotificationExtension::~NotificationExtension() {}

common::Instance* NotificationExtension::CreateInstance() {
#if defined(GENERIC_DESKTOP)
  return new NotificationInstanceDesktop;
#elif defined(TIZEN_MOBILE)
  return new NotificationInstanceMobile(&manager_);
#endif
  return NULL;
}

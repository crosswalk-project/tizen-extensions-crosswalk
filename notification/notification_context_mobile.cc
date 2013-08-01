// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "notification/notification_context.h"
#include "common/picojson.h"

// static
void NotificationContext::PlatformInitialize() {}

NotificationContext::NotificationContext(ContextAPI* api)
    : api_(api) {}

NotificationContext::~NotificationContext() {}

void NotificationContext::HandlePost(const picojson::value& msg) {}

void NotificationContext::HandleRemove(const picojson::value& msg) {}

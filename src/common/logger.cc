// Copyright 2015 Samsung Electronics Co, Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/logger.h"

LogMessage::LogMessage(const char* file, const char* function, int line,
    log_priority priority)
    : file_(file),
      function_(function),
      line_(line),
      priority_(priority) {}

LogMessage::~LogMessage() {
  __dlog_print(LOG_ID_MAIN, priority_, LOGGER_TAG, "%s: %s(%d) > %s",
      file_, function_, line_, stream_.str().c_str());
}

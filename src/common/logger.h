// Copyright 2015 Samsung Electronics Co, Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMMON_LOGGER_H_
#define COMMON_LOGGER_H_

#include <dlog.h>
#include <sstream>

#include "common/utils.h"

#undef LOGGER_TAG
#define LOGGER_TAG "WEB_API"

// A few definitions of macros that don't generate much code. These are used
// by LOGGER() and LOGGER_IF, etc. Since these are used all over our code, it's
// better to have compact code for these operations.
#ifdef TIZEN_DEBUG_ENABLE
#define COMPACT_LOG_DEBUG \
    LogMessage(__MODULE__, __func__, __LINE__, DLOG_DEBUG).stream()
#else
#define COMPACT_LOG_DEBUG \
    true ? (void) 0 : LogMessageVoidify() & (std::ostringstream())
#endif

#define COMPACT_LOG_INFO \
    LogMessage(__MODULE__, __func__, __LINE__, DLOG_INFO).stream()
#define COMPACT_LOG_WARN \
    LogMessage(__MODULE__, __func__, __LINE__, DLOG_WARN).stream()
#define COMPACT_LOG_ERROR \
    LogMessage(__MODULE__, __func__, __LINE__, DLOG_ERROR).stream()

#define LOGGER(priority) COMPACT_LOG_ ## priority
#define LOGGER_IF(priority, condition) \
    !(condition) ? (void) 0 : LogMessageVoidify() & (LOGGER(priority))

// This class more or less represents a particular log message.
// You create an instance of LogMessage and then stream stuff to it.
// When you finish streaming to it, ~LogMessage is called and the
// full message gets streamed to dlog.
//
// You shouldn't actually use LogMessage's constructor to log things,
// though. You should use the LOGGER() macro (and variants thereof) above.
class LogMessage {
 public:
  LogMessage(const char* file, const char* function, int line,
      log_priority priority);
  ~LogMessage();

  std::ostream& stream() { return stream_; }

 private:
  const char* file_;
  const char* function_;
  const int line_;
  log_priority priority_;

  std::ostringstream stream_;

  DISALLOW_COPY_AND_ASSIGN(LogMessage);
};

// This class is used to explicitly ignore values in the conditional
// logging macros. This avoids compiler warnings like "value computed
// is not used" and "statement has no effect".
class LogMessageVoidify {
 public:
  LogMessageVoidify() {}

  // This has to be an operator with a precedence lower than << but
  // higher than ?:
  void operator&(std::ostream &) {}
};

#endif  // COMMON_LOGGER_H_

// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AUDIOSYSTEM_AUDIOSYSTEM_LOGS_H_
#define AUDIOSYSTEM_AUDIOSYSTEM_LOGS_H_

#ifndef NDEBUG

#include <time.h>
#include <fstream>

#define ENABLE_DEBUG 1

// file logging
extern std::ofstream _as_f_log;

#define LOG_INIT()   _as_f_log.open("xwalk_ext_audiosystem.log")
#define LOG_CLOSE()  _as_f_log.close()

#define LOG(type, msg, args...) \
{\
  char exp_msg[2048]; \
  time_t t; \
  struct tm lt; \
  char str_time[64]; \
  time(&t); \
  localtime_r(&t, &lt); \
  strftime(str_time, sizeof(str_time) - 1, "%b %d %H:%M:%S ", &lt); \
  snprintf(exp_msg, sizeof(exp_msg), msg, ##args); \
  _as_f_log << str_time << "[" << type << "] " \
  << __PRETTY_FUNCTION__ << ":" \
  << __LINE__ << ": " << exp_msg << "\n"; \
  _as_f_log.flush(); \
}

#define ERR(msg, args...)  LOG("Error", msg, ##args)
#define WARN(msg, args...)  LOG("Warning", msg, ##args)

#ifdef ENABLE_DEBUG
#   define DBG(msg, args...)  LOG("Debug",  msg, ##args)
#else
#   define DBG   do { } while (0)
#endif  // ENABLE_DEBUG

#else  // NDEBUG

#define ERR(msg, args...)  do { } while (0)
#define WARN(msg, args...) do { } while (0)
#define DBG(msg, args...)  do { } while (0)

#define LOG_INIT()  do { } while (0)
#define LOG_CLOSE() do { } while (0)

#endif  // NDEBUG

#endif  // AUDIOSYSTEM_AUDIOSYSTEM_LOGS_H_

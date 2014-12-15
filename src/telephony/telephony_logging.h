// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TELEPHONY_TELEPHONY_LOGGING_H_
#define TELEPHONY_TELEPHONY_LOGGING_H_

#define LOG_ERR(x) do { std::cout << "[Error] " << x << std::endl; } while (0)

#ifdef NDEBUG
  #define LOG_DBG(x) do {} while (0)
#else
  #define LOG_DBG(x) do { std::cout << "[DBG] " << x << std::endl; } while (0)
#endif

#endif  // TELEPHONY_TELEPHONY_LOGGING_H_

// Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DATASYNC_DATASYNC_LOG_H_
#define DATASYNC_DATASYNC_LOG_H_

#include <iostream>

// TODO(t.iwanek): this header is to be removed
// when some common logging mechanism will be introduced

#define LogInfo(MESSAGE) \
  { std::cerr << MESSAGE << std::endl; }
#define LogDebug(MESSAGE) \
  { std::cerr << MESSAGE << std::endl; }
#define LogWarning(MESSAGE) \
  { std::cerr << MESSAGE << std::endl; }
#define LogError(MESSAGE) \
  { std::cerr << MESSAGE << std::endl; }

#endif  // DATASYNC_DATASYNC_LOG_H_

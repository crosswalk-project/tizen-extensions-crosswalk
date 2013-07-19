// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "time/time_context.h"

DEFINE_XWALK_EXTENSION(TimeContext)

const char TimeContext::name[] = "tizen.time";

// This will be generated from time_api.js.
extern const char kSource_time_api[];

const char* TimeContext::GetJavaScript() {
  return kSource_time_api;
}

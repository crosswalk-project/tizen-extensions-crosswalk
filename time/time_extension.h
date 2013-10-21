// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TIME_TIME_EXTENSION_H_
#define TIME_TIME_EXTENSION_H_

#include "common/extension.h"

class TimeExtension : public common::Extension {
 public:
  TimeExtension();
  virtual ~TimeExtension();

 private:
  // common::Extension implementation.
  virtual common::Instance* CreateInstance();
};

#endif  // TIME_TIME_EXTENSION_H_

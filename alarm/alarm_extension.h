// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ALARM_ALARM_EXTENSION_H_
#define ALARM_ALARM_EXTENSION_H_

#include "common/extension.h"

class AlarmExtension : public common::Extension {
 public:
  AlarmExtension();
  virtual ~AlarmExtension();

 private:
  virtual common::Instance* CreateInstance();
};

#endif  // ALARM_ALARM_EXTENSION_H_

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MESSAGEPORT_MESSAGEPORT_EXTENSION_H_
#define MESSAGEPORT_MESSAGEPORT_EXTENSION_H_

#include "common/extension.h"

class MessageportExtension : public common::Extension {
 public:
  MessageportExtension();
 private:
  // common::Extension implementation.
  virtual common::Instance* CreateInstance();
};

#endif  // MESSAGEPORT_MESSAGEPORT_EXTENSION_H_

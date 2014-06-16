// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PHONE_PHONE_EXTENSION_H_
#define PHONE_PHONE_EXTENSION_H_

#include "common/extension.h"

class PhoneExtension : public common::Extension {
 public:
  PhoneExtension();
  virtual ~PhoneExtension();

 private:
  // common::Extension implementation
  virtual common::Instance* CreateInstance();
};

#endif  // PHONE_PHONE_EXTENSION_H_

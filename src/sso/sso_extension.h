// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SSO_SSO_EXTENSION_H_
#define SSO_SSO_EXTENSION_H_

#include "common/extension.h"

class SSOExtension: public common::Extension {
 public:
  SSOExtension();
  virtual ~SSOExtension();

 private:
  // common::Extension implementation.
  virtual common::Instance* CreateInstance();
};

#endif  // SSO_SSO_EXTENSION_H_

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_EXTENSION_H_
#define SYSTEM_INFO_SYSTEM_INFO_EXTENSION_H_

#include "common/extension.h"

class SystemInfoExtension : public common::Extension {
 public:
  SystemInfoExtension();
  virtual ~SystemInfoExtension();

 private:
  // common::Extension implementation.
  virtual common::Instance* CreateInstance();

  bool initialized_;
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_EXTENSION_H_

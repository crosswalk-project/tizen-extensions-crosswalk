// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TELEPHONY_TELEPHONY_EXTENSION_H_
#define TELEPHONY_TELEPHONY_EXTENSION_H_

#include "common/extension.h"

class TelephonyExtension : public common::Extension {
 public:
  TelephonyExtension();
  virtual ~TelephonyExtension();

 private:
  // common::Extension implementation.
  virtual common::Instance* CreateInstance();
};

#endif  // TELEPHONY_TELEPHONY_EXTENSION_H_

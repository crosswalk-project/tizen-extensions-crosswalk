// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef POWER_POWER_EXTENSION_H_
#define POWER_POWER_EXTENSION_H_

#include "common/extension.h"

#if defined(TIZEN_MOBILE)
#include "power/mobile/power_event_source.h"
#endif

class PowerExtension : public common::Extension {
 public:
  PowerExtension();
  virtual ~PowerExtension();

 private:
  // common::Extension implementation.
  virtual common::Instance* CreateInstance();

#if defined(TIZEN_MOBILE)
  PowerEventSource power_event_source_;
#endif
};

#endif  // POWER_POWER_EXTENSION_H_

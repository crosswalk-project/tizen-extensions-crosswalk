// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CONTENT_EXTENSION_H_
#define CONTENT_CONTENT_EXTENSION_H_

#include "common/extension.h"

class ContentExtension : public common::Extension {
 public:
  ContentExtension();
  virtual ~ContentExtension();

 private:
  virtual common::Instance* CreateInstance();
};

#endif  // CONTENT_CONTENT_EXTENSION_H_

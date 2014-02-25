// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIASERVER_MEDIASERVER_EXTENSION_H_
#define MEDIASERVER_MEDIASERVER_EXTENSION_H_

#include "common/extension.h"

class MediaServerExtension : public common::Extension {
 public:
  MediaServerExtension();
  virtual ~MediaServerExtension();

 private:
  // common::Extension implementation.
  virtual common::Instance* CreateInstance();
};

#endif  // MEDIASERVER_MEDIASERVER_EXTENSION_H_

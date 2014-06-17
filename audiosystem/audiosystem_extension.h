// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AUDIOSYSTEM_AUDIOSYSTEM_EXTENSION_H_
#define AUDIOSYSTEM_AUDIOSYSTEM_EXTENSION_H_

#include "common/extension.h"

class AudioSystemExtension : public common::Extension {
 public:
  AudioSystemExtension();
  ~AudioSystemExtension();
 private:
  // common::Extension implementation.
  virtual common::Instance* CreateInstance();
};

#endif  // AUDIOSYSTEM_AUDIOSYSTEM_EXTENSION_H_

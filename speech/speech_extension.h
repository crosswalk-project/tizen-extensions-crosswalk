// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SPEECH_SPEECH_EXTENSION_H_
#define SPEECH_SPEECH_EXTENSION_H_

#include "common/extension.h"

class SpeechExtension : public common::Extension {
 public:
  SpeechExtension();
  virtual ~SpeechExtension();

 private:
  // common::Extension implementation
  virtual common::Instance* CreateInstance();
};

#endif  // SPEECH_SPEECH_EXTENSION_H_

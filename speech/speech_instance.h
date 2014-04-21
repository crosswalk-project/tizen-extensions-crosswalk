// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SPEECH_SPEECH_INSTANCE_H_
#define SPEECH_SPEECH_INSTANCE_H_

#include "common/extension.h"
#include "common/picojson.h"

class SpeechInstance : public common::Instance {
 public:
  SpeechInstance();
  virtual ~SpeechInstance();

 private:
  // common::Instance implementation.
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* message);

  const picojson::value::object
  HandleVocalizeString(const picojson::value& msg);
};

#endif  // SPEECH_SPEECH_INSTANCE_H_

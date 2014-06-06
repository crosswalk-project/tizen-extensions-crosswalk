// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SPEECH_SPEECH_INSTANCE_H_
#define SPEECH_SPEECH_INSTANCE_H_

#include <glib.h>
#include <thread>  // NOLINT

#include "common/extension.h"
#include "common/picojson.h"
#include "speech/tizen_srs_gen.h"

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
  const picojson::value::object
      HandleListenVoice(const picojson::value& msg);

  static void ProxyResultCb(TizenSrs *, const gchar *const *, void*);

  TizenSrs*    tts_proxy_;  // text to speech proxy
  TizenSrs*    stt_proxy_;  // speech to text proxy
#ifdef TIZEN
  static void SetupMainloop(void *data);
  GMainLoop*   main_loop_;
  std::thread  thread_;
#endif  // TIZEN
};

#endif  // SPEECH_SPEECH_INSTANCE_H_

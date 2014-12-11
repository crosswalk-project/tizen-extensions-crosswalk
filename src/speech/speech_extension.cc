// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "speech/speech_extension.h"

#include "speech/speech_instance.h"

common::Extension* CreateExtension() {
  return new SpeechExtension();
}

// This will be generated from speech_api.js
extern const char kSource_speech_api[];

SpeechExtension::SpeechExtension() {
  SetExtensionName("tizen.speech");
  SetJavaScriptAPI(kSource_speech_api);
}

SpeechExtension::~SpeechExtension() {}

common::Instance* SpeechExtension::CreateInstance() {
  return new SpeechInstance;
}

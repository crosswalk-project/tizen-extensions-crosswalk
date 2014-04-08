// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "speech/speech_instance.h"

#include <gio/gio.h>
#include <string>

SpeechInstance::SpeechInstance() {}

SpeechInstance::~SpeechInstance() {}

void SpeechInstance::HandleMessage(const char* message) {}

void SpeechInstance::HandleSyncMessage(const char* message) {
  picojson::value v;
  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty())
    return;

  picojson::value::object o;
  std::string cmd = v.get("cmd").to_str();
  if (cmd == "VocalizeString")
    o = HandleVocalizeString(v);

  SendSyncReply(picojson::value(o).serialize().c_str());
}

const picojson::value::object SpeechInstance::HandleVocalizeString(
    const picojson::value& msg) {
  picojson::value::object o;

  std::string speakText = msg.get("text").to_str();
  g_dbus_connection_call(g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL),
      "org.tizen.srs",
      "/tts",
      "org.tizen.srs",
      "synthesize",
      g_variant_new("(ss)", speakText.c_str(), "english"),
      NULL,
      G_DBUS_CALL_FLAGS_NONE,
      -1,
      NULL,
      NULL,
      NULL);

  o["text"] = picojson::value("");
  return o;
}

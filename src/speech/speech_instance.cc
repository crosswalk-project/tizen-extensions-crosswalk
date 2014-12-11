// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "speech/speech_instance.h"

#include <gio/gio.h>
#include <string>

#ifdef TIZEN
// static
void SpeechInstance::SetupMainloop(void* data) {
  SpeechInstance* self = reinterpret_cast<SpeechInstance*>(data);

  GMainContext* ctx = g_main_context_default();

  g_main_context_push_thread_default(ctx);

  g_main_loop_run(self->main_loop_);
}
#endif  // TIZEN

// static
void SpeechInstance::ProxyResultCb(TizenSrs* proxy,
    const gchar* const* tokens, void* data) {
  SpeechInstance* self = reinterpret_cast<SpeechInstance*>(data);
  picojson::array cmds;

  for (int i = 0; tokens[i]; i++) {
    cmds.push_back(picojson::value(tokens[i]));
  }

  picojson::value::object m;
  m["cmd"] = picojson::value("FoundCommands");
  m["commands"] = picojson::value(cmds);

  self->PostMessage(picojson::value(m).serialize().c_str());
}

SpeechInstance::SpeechInstance()
    : tts_proxy_(0)
    , stt_proxy_(0)
#ifdef TIZEN
    , main_loop_(g_main_loop_new(0, FALSE))
    , thread_(SpeechInstance::SetupMainloop, this) {
  thread_.detach();
#else
{
#endif  // TIZEN
}

SpeechInstance::~SpeechInstance() {
#ifdef TIZEN
  g_main_loop_quit(main_loop_);
  g_main_loop_unref(main_loop_);
#endif  // TIZEN
  if (tts_proxy_) g_clear_object(&tts_proxy_);
  if (stt_proxy_) g_clear_object(&stt_proxy_);
}

void SpeechInstance::HandleMessage(const char* message) {}

void SpeechInstance::HandleSyncMessage(const char* message) {
  picojson::value v;
  std::string err;

  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty())
    return;

  picojson::value::object o;
  const std::string& cmd = v.get("cmd").to_str();
  if (cmd == "VocalizeString")
    o = HandleVocalizeString(v);
  else if (cmd == "ListenVoice")
    o = HandleListenVoice(v);

  SendSyncReply(picojson::value(o).serialize().c_str());
}

const picojson::value::object SpeechInstance::HandleVocalizeString(
    const picojson::value& msg) {
  picojson::value::object o;
  guint id = 0;
  GError* err = 0;
  std::string tmp_err;

  if (!tts_proxy_) {
    // try once again if server is available
    tts_proxy_ = tizen_srs_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
        (GDBusProxyFlags)G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
         "org.tizen.srs", "/tts", 0, &err);
  }

  if (!tts_proxy_) {
    tmp_err = "Failed to connect to server : " + std::string(err->message);
    o["error"] = picojson::value(true);
    o["errorMsg"] = picojson::value(tmp_err);
    return o;
  }

  const std::string& speak_text = msg.get("text").to_str();
  const std::string& language = msg.get("language").to_str();

  tizen_srs_call_synthesize_sync(tts_proxy_, speak_text.c_str(),
      language.c_str(), &id, 0, &err);

  if (err) {
    tmp_err = "Failed to vocalize: " + std::string(err->message);
    o["error"] = picojson::value(true);
    o["errorMsg"] = picojson::value(tmp_err);
    g_error_free(err);
  }

  return o;
}

const picojson::value::object SpeechInstance::HandleListenVoice(
    const picojson::value& msg) {
  bool listen = msg.get("listen").get<bool>();
  picojson::value::object o;

  if (listen) {
    if (!stt_proxy_) {
      GError* err = 0;

      stt_proxy_ = tizen_srs_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
         G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
         "org.tizen.srs", "/srs", 0, &err);
      if (err) {
        o["error"] = picojson::value(true);
        o["errorMsg"] = picojson::value(err->message);
        g_error_free(err);
      } else {
        g_signal_connect(stt_proxy_, "result",
            G_CALLBACK(SpeechInstance::ProxyResultCb), this);
      }
    }
  } else if (stt_proxy_) {
    g_clear_object(&stt_proxy_);
  }

  return o;
}

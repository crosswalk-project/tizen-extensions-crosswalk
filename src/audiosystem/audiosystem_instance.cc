// Copyright(c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "audiosystem/audiosystem_instance.h"

#include <exception>
#include <map>
#include <string>

#include "audiosystem/audiosystem_context.h"
#include "audiosystem/audiosystem_logs.h"
#include "common/picojson.h"

// Singleton context
std::shared_ptr<AudioSystemContext>
    AudioSystemInstance::context_(new AudioSystemContext());
uint32_t AudioSystemInstance::instance_counter_ = 0;
std::thread* AudioSystemInstance::t_ = 0;
std::mutex AudioSystemInstance::mtx_;

// FIXME: xwalk extension framework does not support
// adding io/timer/idle events to it's main loop, so as a temporary fix,
// running pulseaudio main loop in a thread to receive events
// from pulseaudio.
// https://crosswalk-project.org/jira/browse/XWALK-1331
// static
void AudioSystemInstance::RunLoop() {
  int r;
  while (1) {
    mtx_.lock();
    r = context_->Prepare();
    mtx_.unlock();
    if (r < 0) break;

    r = context_->Poll();
    if (r < 0) break;

    mtx_.lock();
    r = context_->Dispatch();
    mtx_.unlock();
    if (r < 0) break;
  }
}

// static
void AudioSystemInstance::InitContext() {
  if (!context_.get())
    context_.reset(new AudioSystemContext());

  if (!t_)
    t_ = new std::thread(AudioSystemInstance::RunLoop);
}

// static
void AudioSystemInstance::DeInitContext() {
  context_->QuitLoop();
  t_->join();
  delete t_;
  t_ = 0;
  context_.reset();
}

AudioSystemInstance::AudioSystemInstance() {
  DBG("Creating audiosystem instance");
  if (++instance_counter_ == 1) {
    AudioSystemInstance::InitContext();
  }
}

AudioSystemInstance::~AudioSystemInstance() {
  DBG("Deleting audiosystem instance");
  if (--instance_counter_ == 0) {
    AudioSystemInstance::DeInitContext();
  }
}

void AudioSystemInstance::HandleMessage(const char* message) {
  picojson::value js_message;
  std::string err;

  picojson::parse(js_message, message, message + strlen(message), &err);
  if (!err.empty()) {
    ERR("Message parsing error: '%s', Error was : %s", message, err.c_str());
  } else if (!js_message.contains("cmd") || !js_message.contains("req_id")) {
    err = "Mandatory parameters(cmd, req_id) missing from message";
    ERR("%s", err.c_str());
  }

  if (!err.empty()) {
    picojson::value::object js_reply;

    js_reply["req_id"] = js_message.get("req_id");
    js_reply["error"] = picojson::value(true);
    js_reply["errorMsg"] = picojson::value(err);

    PostMessage(js_reply);
    return;
  }

  AudioSystemInstance::mtx_.lock();
  context_->HandleMessage(js_message, this);
  AudioSystemInstance::mtx_.unlock();
}

void AudioSystemInstance::HandleSyncMessage(const char* message) {
  picojson::value js_message;
  std::string err;

  picojson::parse(js_message, message, message + strlen(message), &err);
  if (!err.empty()) {
    ERR("Message parsing error: '%s', Error was : %s", message, err.c_str());
  } else if (!js_message.contains("cmd")) {
    err = "Mandatory parameters(cmd) missing from message";
    ERR("%s", err.c_str());
  }

  picojson::value::object js_reply;
  if (!err.empty()) {
    js_reply["error"] = picojson::value(true);
    js_reply["errorMsg"] = picojson::value(err);
  } else {
    AudioSystemInstance::mtx_.lock();
    js_reply = context_->HandleSyncMessage(js_message);
    AudioSystemInstance::mtx_.unlock();
  }

  SendSyncReply(js_reply);
}

void AudioSystemInstance::PostMessage(const picojson::value& js_message) {
  Instance::PostMessage(js_message.serialize().c_str());
}

void AudioSystemInstance::PostMessage(
    const picojson::value::object& js_message) {
  picojson::value js_value = picojson::value(js_message);
  PostMessage(js_value);
}

void AudioSystemInstance::SendSyncReply(
    const picojson::value::object& js_message) {
  picojson::value js_value = picojson::value(js_message);

  Instance::SendSyncReply(js_value.serialize().c_str());
}

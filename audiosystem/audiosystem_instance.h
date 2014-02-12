// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AUDIOSYSTEM_AUDIOSYSTEM_INSTANCE_H_
#define AUDIOSYSTEM_AUDIOSYSTEM_INSTANCE_H_

#include <memory>
#include <mutex>  // NOLINT
#include <thread>  // NOLINT

#include "common/extension.h"
#include "common/picojson.h"

class AudioSystemContext;

class AudioSystemInstance: public common::Instance {
 public:
  AudioSystemInstance();
  ~AudioSystemInstance();

  void PostMessage(const picojson::value::object& js_message);
  void PostMessage(const picojson::value& js_message);
  void SendSyncReply(const picojson::value::object& js_message);

 private:
  // common::Instance implementation.
  virtual void HandleMessage(const char* message);
  virtual void HandleSyncMessage(const char* message);

  // Singleton Context preperations
  static void RunLoop();
  static void InitContext();
  static void DeInitContext();

  static uint32_t instance_counter_;
  static std::shared_ptr<AudioSystemContext> context_;
  static std::mutex mtx_;
  static std::thread* t_;
};

#endif  // AUDIOSYSTEM_AUDIOSYSTEM_INSTANCE_H_

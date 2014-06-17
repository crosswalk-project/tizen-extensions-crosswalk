// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AUDIOSYSTEM_AUDIOSYSTEM_MUTE_CONTROL_H_
#define AUDIOSYSTEM_AUDIOSYSTEM_MUTE_CONTROL_H_

#include <pulse/ext-volume-api.h>
#include <pulse/pulseaudio.h>
#include <map>
#include <string>
#include <vector>

#include "common/picojson.h"

class AudioSystemContext;
class AudioSystemInstance;

class MuteControl {
 public:
  MuteControl(AudioSystemContext* parent,
              const pa_ext_volume_api_mute_control_info& info);
  ~MuteControl();

  uint32_t index() const {
    return index_;
  }

  const std::string& label() const {
    return label_;
  }

  bool muted() const {
    return muted_;
  }

  void ToggleMute(AudioSystemInstance* caller, const std::string& req_id);
  void UpdateInfo(const pa_ext_volume_api_mute_control_info& info);
  picojson::object ToJsonObject() const;

 private:
  static void PaSetMuteCb(pa_context* c, int success, void* userdata);

  AudioSystemContext* parent_;
  uint32_t index_;
  std::string label_;
  bool muted_;
};

#endif  // AUDIOSYSTEM_AUDIOSYSTEM_MUTE_CONTROL_H_

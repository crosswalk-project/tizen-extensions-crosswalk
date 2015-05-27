// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AUDIOSYSTEM_AUDIOSYSTEM_AUDIO_GROUP_H_
#define AUDIOSYSTEM_AUDIOSYSTEM_AUDIO_GROUP_H_

#include <pulse/ext-volume-api.h>
#include <pulse/pulseaudio.h>
#include <string>

#include "common/picojson.h"

class AudioSystemContext;

class AudioGroup {
 public:
  AudioGroup(AudioSystemContext* parent,
             const pa_ext_volume_api_audio_group_info& info);

  ~AudioGroup();

  uint32_t index() const {
    return index_;
  }

  const std::string& label() const {
    return label_;
  }

  uint32_t volume_control() const {
    return volume_control_;
  }

  uint32_t mute_control() const {
    return mute_control_;
  }

  void UpdateInfo(const pa_ext_volume_api_audio_group_info& info);
  picojson::object ToJsonObject() const;

 private:
  AudioSystemContext* parent_;
  uint32_t index_;
  std::string label_;
  std::string name_;
  uint32_t volume_control_;
  uint32_t mute_control_;
};

#endif  // AUDIOSYSTEM_AUDIOSYSTEM_AUDIO_GROUP_H_

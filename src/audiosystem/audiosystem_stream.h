// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AUDIOSYSTEM_AUDIOSYSTEM_STREAM_H_
#define AUDIOSYSTEM_AUDIOSYSTEM_STREAM_H_

#include <pulse/ext-volume-api.h>
#include <pulse/pulseaudio.h>
#include <string>

#include "common/picojson.h"

class AudioSystemContext;

class AudioStream {
 public:
  AudioStream(AudioSystemContext* parent,
              const pa_ext_volume_api_stream_info& info);
  ~AudioStream();

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

  pa_direction_t direction() const {
    return direction_;
  }

  void UpdateInfo(const pa_ext_volume_api_stream_info& info);
  picojson::object ToJsonObject() const;

 private:
  AudioSystemContext* parent_;
  uint32_t index_;
  std::string label_;
  uint32_t volume_control_;
  uint32_t mute_control_;
  pa_direction_t direction_;
};

#endif  // AUDIOSYSTEM_AUDIOSYSTEM_STREAM_H_

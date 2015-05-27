// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AUDIOSYSTEM_AUDIOSYSTEM_DEVICE_H_
#define AUDIOSYSTEM_AUDIOSYSTEM_DEVICE_H_

#include <pulse/ext-volume-api.h>
#include <pulse/pulseaudio.h>
#include <string>
#include <vector>

#include "common/picojson.h"

class AudioSystemContext;

class AudioDevice {
 public:
  AudioDevice(AudioSystemContext* parent,
              const pa_ext_volume_api_device_info& info);
  ~AudioDevice();

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

  const std::vector<std::string>& types() const {
    return types_;
  }

  void UpdateInfo(const pa_ext_volume_api_device_info& info);
  picojson::object ToJsonObject() const;

 private:
  AudioSystemContext* parent_;
  uint32_t index_;
  std::string label_;
  uint32_t volume_control_;
  uint32_t mute_control_;
  pa_direction_t direction_;
  std::vector<std::string> types_;
};

#endif  // AUDIOSYSTEM_AUDIOSYSTEM_DEVICE_H_

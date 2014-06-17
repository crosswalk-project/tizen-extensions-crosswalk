// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AUDIOSYSTEM_AUDIOSYSTEM_VOLUME_CONTROL_H_
#define AUDIOSYSTEM_AUDIOSYSTEM_VOLUME_CONTROL_H_

#include <pulse/ext-volume-api.h>
#include <pulse/pulseaudio.h>
#include <map>
#include <string>
#include <vector>

#include "common/picojson.h"

typedef pa_ext_volume_api_bvolume pa_bvolume;
class AudioSystemContext;
class AudioSystemInstance;

class VolumeControl {
 public:
  enum Flags {
    NONE = 0x00,
    LABEL = 0x01,
    VOLUME = 0x02,
    BALANCE = 0x04,
    CHANNELS = 0x08,
    ALL = LABEL | VOLUME | BALANCE | CHANNELS
  };

  VolumeControl(AudioSystemContext* parent,
                const pa_ext_volume_api_volume_control_info& info);

  ~VolumeControl();

  uint32_t index() const {
    return index_;
  }

  const std::string& label() const {
    return label_;
  }

  const pa_bvolume& volume() const {
    return volume_;
  }

  VolumeControl::Flags UpdateInfo(
      const pa_ext_volume_api_volume_control_info& info);
  void SetVolume(double volume,
                 AudioSystemInstance* caller,
                 const std::string& req_id);
  void SetBalance(const std::vector<double>& balance,
                  AudioSystemInstance* caller,
                  const std::string& req_id);
  void SetSimplifiedBalance(double balance,
                            double fade,
                            AudioSystemInstance* caller,
                            const std::string& req_id);
  picojson::value::object GetSimplifiedBalance() const;

  picojson::object ToJsonObject() const;

 private:
  picojson::array FlagsToJSONArray(VolumeControl::Flags flags) const;
  void EmitChanged(VolumeControl::Flags change) const;
  static void PaSetVolumeCb(pa_context* c, int success, void* userdata);

  AudioSystemContext* parent_;
  uint32_t index_;
  std::string label_;
  pa_bvolume volume_;
  std::vector<double> volume_steps_;
};

#endif  // AUDIOSYSTEM_AUDIOSYSTEM_VOLUME_CONTROL_H_

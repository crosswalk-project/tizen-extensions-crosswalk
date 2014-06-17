// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "audiosystem/audiosystem_device.h"

#include "audiosystem/audiosystem_context.h"
#include "audiosystem/audiosystem_logs.h"

AudioDevice::AudioDevice(
    AudioSystemContext* parent,
    const pa_ext_volume_api_device_info& info)
    : parent_(parent),
      index_(info.index),
      label_(info.description),
      volume_control_(info.volume_control),
      mute_control_(info.mute_control),
      direction_(info.direction) {
  for (int i = 0; i < info.n_device_types; i++)
    types_.push_back(info.device_types[i]);
}

AudioDevice::~AudioDevice() {
}

void AudioDevice::UpdateInfo(
    const pa_ext_volume_api_device_info& info) {
  picojson::object js_message;
  picojson::array  changes;

  if (label_ != info.description) {
    label_ = info.description;
    changes.push_back(picojson::value("label"));
  }

  if (volume_control_ != info.volume_control) {
    volume_control_ = info.volume_control;
    changes.push_back(picojson::value("volume_control"));
  }

  if (mute_control_ != info.mute_control) {
    mute_control_ = info.mute_control;
    changes.push_back(picojson::value("mute_control"));
  }

  js_message["cmd"] = picojson::value("device_changed");
  js_message["device"] = picojson::value(ToJsonObject());
  js_message["changes"] = picojson::value(changes);

  parent_->PostMessage(js_message);
}

picojson::value::object AudioDevice::ToJsonObject() const {
  picojson::value::object js_reply;
  const std::string& str_direction =
      (direction_ == (PA_DIRECTION_OUTPUT | PA_DIRECTION_INPUT))
      ? "bidirectional" : (direction_ & PA_DIRECTION_OUTPUT)
      ? "output" : "input";

  js_reply["id"] = picojson::value(static_cast<double>(index_));
  js_reply["label"] = picojson::value(label_);
  js_reply["direction"] = picojson::value(str_direction);
  if (volume_control_ != PA_INVALID_INDEX) {
    js_reply["volume_control"] = picojson::value(
        static_cast<double>(volume_control_));
  }

  if (mute_control_ != PA_INVALID_INDEX) {
    js_reply["mute_control"] = picojson::value(
        static_cast<double>(mute_control_));
  }

  picojson::value::array types_array;
  std::vector<std::string>::const_iterator it;
  for (it = types_.begin(); it != types_.end(); it++)
    types_array.push_back(picojson::value(*it));

  js_reply["device_types"] = picojson::value(types_array);

  return js_reply;
}

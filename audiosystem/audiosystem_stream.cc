// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "audiosystem/audiosystem_stream.h"

#include "audiosystem/audiosystem_context.h"
#include "audiosystem/audiosystem_logs.h"

AudioStream::AudioStream(
    AudioSystemContext* parent,
    const pa_ext_volume_api_stream_info& info)
    : parent_(parent),
      index_(info.index),
      label_(info.description),
      volume_control_(info.volume_control),
      mute_control_(info.mute_control),
      direction_(info.direction) {
}

AudioStream::~AudioStream() {
}

void AudioStream::UpdateInfo(
    const pa_ext_volume_api_stream_info& info) {
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

  js_message["cmd"] = picojson::value("audiostreamchanged");
  js_message["stream"] = picojson::value(ToJsonObject());
  js_message["changes"] = picojson::value(changes);

  parent_->PostMessage(js_message);
}

picojson::value::object AudioStream::ToJsonObject() const {
  picojson::value::object js_reply;

  js_reply["id"] = picojson::value(static_cast<double>(index_));
  js_reply["label"] = picojson::value(label_);
  js_reply["direction"] = picojson::value(
      direction_ == PA_DIRECTION_INPUT ? "input" :
      direction_ == PA_DIRECTION_OUTPUT ? "output" : "bidirectional");

  if (volume_control_ != PA_INVALID_INDEX) {
    js_reply["volume_control"] = picojson::value(
        static_cast<double>(volume_control_));
  }

  if (mute_control_ != PA_INVALID_INDEX) {
    js_reply["mute_control"] = picojson::value(
        static_cast<double>(mute_control_));
  }

  return js_reply;
}

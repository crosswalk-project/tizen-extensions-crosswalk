// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "audiosystem/audiosystem_audio_group.h"

#include "audiosystem/audiosystem_context.h"
#include "audiosystem/audiosystem_logs.h"

AudioGroup::AudioGroup(
    AudioSystemContext* parent,
    const pa_ext_volume_api_audio_group_info& info)
    : parent_(parent),
      index_(info.index),
      label_(info.name),
      name_(info.description),
      volume_control_(info.volume_control),
      mute_control_(info.mute_control) {
}

AudioGroup::~AudioGroup() {
}

void AudioGroup::UpdateInfo(
    const pa_ext_volume_api_audio_group_info& info) {
  picojson::object js_message;
  picojson::array  changes;

  if (label_ != info.name) {
    label_ = info.name;
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

  js_message["cmd"] = picojson::value("group_changed");
  js_message["group"] = picojson::value(ToJsonObject());
  js_message["changes"] = picojson::value(changes);

  parent_->PostMessage(js_message);
}

picojson::value::object AudioGroup::ToJsonObject() const {
  picojson::value::object reply;

  reply["id"] = picojson::value(static_cast<double>(index_));
  reply["label"] = picojson::value(label_);
  reply["name"] = picojson::value(name_);
  if (volume_control_ != PA_INVALID_INDEX)
    reply["volume_control"] = picojson::value(
        static_cast<double>(volume_control_));
  if (mute_control_ != PA_INVALID_INDEX)
    reply["mute_control"] = picojson::value(
        static_cast<double>(mute_control_));

  return reply;
}

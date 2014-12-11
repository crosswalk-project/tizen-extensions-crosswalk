// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "audiosystem/audiosystem_volume_control.h"

#include "audiosystem/audiosystem_context.h"
#include "audiosystem/audiosystem_instance.h"
#include "audiosystem/audiosystem_logs.h"

#define PA_VOLUME_TO_DOUBLE(v) (((v)*  100.0 / PA_VOLUME_NORM) / 100.0)
#define DOUBLE_TO_PA_VOLUME(d) (uint32_t)(((d*  100)*  PA_VOLUME_NORM) / 100.0)

VolumeControl::Flags operator|(
    VolumeControl::Flags a, VolumeControl::Flags b) {
  int new_flag  = static_cast<int>(a) | static_cast<int>(b);

  return static_cast<VolumeControl::Flags>(new_flag);
}

struct SetVolumeRequestData {
  SetVolumeRequestData(VolumeControl* ctrl,
      AudioSystemInstance* clr, const std::string& req_id)
      : request_id(req_id), caller(clr), control(ctrl) {
  }
  std::string request_id;
  AudioSystemInstance* caller;
  VolumeControl* control;
};

VolumeControl::VolumeControl(AudioSystemContext* parent,
    const pa_ext_volume_api_volume_control_info& info)
    : parent_(parent),
      index_(info.index),
      label_(info.description),
      volume_(info.volume) {
}

VolumeControl::~VolumeControl() {
  // TODO(avalluri): cancel all pending operations
}

void VolumeControl::PaSetVolumeCb(pa_context* c, int success, void* userdata) {
  SetVolumeRequestData* data =
      reinterpret_cast<SetVolumeRequestData*>(userdata);
  VolumeControl* self = data->control;
  AudioSystemInstance* caller = data->caller;
  picojson::value::object js_reply;

  js_reply["req_id"] = picojson::value(data->request_id);
  delete data;

  if (!success) {
    std::string err = pa_strerror(pa_context_errno(self->parent_->context()));

    WARN("Failed to set balance: %s",  err.c_str());

    js_reply["error"] = picojson::value(true);
    js_reply["errorMsg"] = picojson::value(err);
  }

  caller->PostMessage(js_reply);
}

void VolumeControl::SetVolume(double volume, AudioSystemInstance* caller,
    const std::string& req_id) {
  pa_bvolume bv = volume_;

  bv.volume = DOUBLE_TO_PA_VOLUME(volume);
  SetVolumeRequestData* data = new SetVolumeRequestData(this, caller, req_id);
  pa_operation* op = pa_ext_volume_api_set_volume_control_volume_by_index(
      parent_->context(), index_, &bv, 1, 0, PaSetVolumeCb, data);
  if (!op) {
    picojson::value::object js_reply;

    WARN("Failed to set volume : %s",
        pa_strerror(pa_context_errno(parent_->context())));
    js_reply["req_id"] = picojson::value(req_id);
    js_reply["error"] = picojson::value(true);
    js_reply["errorMsg"] = picojson::value(
        pa_strerror(pa_context_errno(parent_->context())));
    caller->PostMessage(js_reply);
    delete data;
    return;
  }

  // TODO(avalluri): push to pending list?
  pa_operation_unref(op);
}

void VolumeControl::SetBalance(const std::vector<double>& balance,
    AudioSystemInstance* caller, const std::string& req_id) {
  pa_bvolume bv = volume_;

  for (int i = 0; i < bv.channel_map.channels && i < balance.size(); i++) {
    bv.balance[i] = balance[i];
  }

  SetVolumeRequestData* data = new SetVolumeRequestData(this, caller, req_id);
  pa_operation* op = pa_ext_volume_api_set_volume_control_volume_by_index(
      parent_->context(), index_, &bv, 0, 1, PaSetVolumeCb, data);
  if (!op) {
    picojson::value::object js_reply;
    WARN("Failed to set balance : %s",
        pa_strerror(pa_context_errno(parent_->context())));

    js_reply["req_id"] = picojson::value(req_id);
    js_reply["error"] = picojson::value(true);
    js_reply["errorMsg"] = picojson::value(
        pa_strerror(pa_context_errno(parent_->context())));
    caller->PostMessage(js_reply);
    delete data;
    return;
  }

  // TODO(avalluri): push to pending list?
  pa_operation_unref(op);
}

void VolumeControl::SetSimplifiedBalance(double balance, double fade,
    AudioSystemInstance* caller, const std::string& req_id) {
  pa_bvolume bv = volume_;
  pa_cvolume cv;

  pa_cvolume_reset(&cv, bv.channel_map.channels);
  pa_cvolume_set_balance(&cv, &bv.channel_map, balance);
  pa_cvolume_set_fade(&cv, &bv.channel_map, fade);

  for (int i = 0; i < cv.channels; i++) {
    bv.balance[i] = static_cast<double>(cv.values[i]) / PA_VOLUME_NORM;
  }

  SetVolumeRequestData* data = new SetVolumeRequestData(this, caller, req_id);
  pa_operation* op = pa_ext_volume_api_set_volume_control_volume_by_index(
      parent_->context(), index_, &bv, 0, 1, PaSetVolumeCb, data);
  if (!op) {
    picojson::value::object js_reply;

    WARN("Failed to set smimplified balance: %s",
        pa_strerror(pa_context_errno(parent_->context())));

    js_reply["req_id"] = picojson::value(req_id);
    js_reply["error"] = picojson::value(true);
    js_reply["errorMsg"] = picojson::value(
        pa_strerror(pa_context_errno(parent_->context())));
    caller->PostMessage(js_reply);
    delete data;
    return;
  }

  // TODO(avalluri): push to pending list?
  pa_operation_unref(op);
}

picojson::value::object VolumeControl::GetSimplifiedBalance() const {
  pa_cvolume cv;
  pa_ext_volume_api_bvolume_to_cvolume(&volume_, &cv);

  double balance = pa_cvolume_get_balance(&cv, &volume_.channel_map);
  double fade = pa_cvolume_get_fade(&cv, &volume_.channel_map);

  picojson::value::object js_reply;
  js_reply["balance"] = picojson::value(balance);
  js_reply["fade"] = picojson::value(fade);
  return js_reply;
}

VolumeControl::Flags VolumeControl::UpdateInfo(
    const pa_ext_volume_api_volume_control_info &info) {
  Flags change_flags = NONE;

  DBG("Old Volume : %u, new Volume : %u", volume_.volume, info.volume.volume);

  if (label_ != info.description) {
    label_ = info.description;
    change_flags = LABEL;
  }

  if (volume_.volume != info.volume.volume) {
    change_flags = change_flags | VOLUME;
  }

  if (volume_.channel_map.channels != info.volume.channel_map.channels) {
    change_flags = change_flags | BALANCE;
    change_flags = change_flags | CHANNELS;
  }

  if (!(change_flags & CHANNELS)) {
    for (int i = 0; i < info.volume.channel_map.channels; i++) {
      if (volume_.channel_map.map[i] != info.volume.channel_map.map[i]) {
        change_flags = change_flags | CHANNELS;
        break;
      }
    }
  }

  if (!(change_flags & BALANCE)) {
    for (int i = 0; i < info.volume.channel_map.channels; i++) {
      if (volume_.balance[i] != info.volume.balance[i]) {
        change_flags = change_flags | BALANCE;
        break;
      }
    }
  }

  volume_ = info.volume;
  EmitChanged(change_flags);
  return change_flags;
}

picojson::value::object VolumeControl::ToJsonObject() const {
  picojson::value::object reply;
  picojson::array array;

  reply["id"] = picojson::value(static_cast<double>(index_));
  reply["label"] = picojson::value(label_);
  reply["volume"] = picojson::value(PA_VOLUME_TO_DOUBLE(volume_.volume));
  for (int i = 0; i < volume_.channel_map.channels; i++) {
    const char *channel =
        pa_channel_position_to_string(volume_.channel_map.map[i]);
    const char *label =
        pa_channel_position_to_pretty_string(volume_.channel_map.map[i]);
    double balance = volume_.balance[i];

    picojson::value::object dict_entry;
    dict_entry["position"] = picojson::value(channel);
    dict_entry["label"] = picojson::value(label);
    dict_entry["balance"] = picojson::value(balance);

    array.push_back(picojson::value(dict_entry));
  }
  reply["balance"] = picojson::value(array);
  array.clear();

  std::vector<double>::const_iterator it;
  for (it = volume_steps_.begin(); it != volume_steps_.end(); it++)
    array.push_back(picojson::value(static_cast<double>(*it)));
  reply["volume_steps"] = picojson::value(array);
  array.clear();

  return reply;
}

picojson::array VolumeControl::FlagsToJSONArray(Flags flags) const {
  picojson::array changes;

  if (flags & LABEL)
    changes.push_back(picojson::value("label"));

  if (flags & VOLUME)
    changes.push_back(picojson::value("volume"));

  if (flags & BALANCE)
    changes.push_back(picojson::value("balance"));

  if (flags & CHANNELS)
    changes.push_back(picojson::value("channels"));

  return changes;
}

void VolumeControl::EmitChanged(Flags change) const {
  picojson::value::object js_message;

  if (change == NONE)
    return;

  DBG("Change Type :%x", change);

  js_message["cmd"] = picojson::value("volume_control_changed");
  js_message["control"] = picojson::value(ToJsonObject());
  js_message["changes"] = picojson::value(FlagsToJSONArray(change));
  parent_->PostMessage(js_message);
}

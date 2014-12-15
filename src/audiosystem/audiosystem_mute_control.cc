// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "audiosystem/audiosystem_mute_control.h"

#include "audiosystem/audiosystem_context.h"
#include "audiosystem/audiosystem_instance.h"
#include "audiosystem/audiosystem_logs.h"

struct MuteData {
  MuteData(MuteControl* ctrl,
      AudioSystemInstance* clr, const std::string& req_id)
      : control(ctrl), caller(clr), request_id(req_id) {
  }

  MuteControl* control;
  AudioSystemInstance* caller;
  std::string request_id;
};

MuteControl::MuteControl(AudioSystemContext* parent,
    const pa_ext_volume_api_mute_control_info& info)
    : parent_(parent),
      index_(info.index),
      label_(info.description),
      muted_(static_cast<bool>(info.mute)) {
}

MuteControl::~MuteControl() {
}

void MuteControl::PaSetMuteCb(pa_context* c, int success, void* userdata) {
  MuteData* data = reinterpret_cast<MuteData*>(userdata);
  MuteControl* self = data->control;
  AudioSystemInstance* caller = data->caller;
  picojson::value::object js_reply;

  js_reply["req_id"] = picojson::value(data->request_id);

  delete data;

  if (!success) {
    std::string err = pa_strerror(pa_context_errno(self->parent_->context()));

    js_reply["error"] = picojson::value(true);
    js_reply["errorMsg"] = picojson::value(err);
  }

  caller->PostMessage(js_reply);
}

void MuteControl::ToggleMute(AudioSystemInstance* caller,
    const std::string& req_id) {
  MuteData* data = new MuteData(this, caller, req_id);
  pa_operation* op = pa_ext_volume_api_set_mute_control_mute_by_index(
      parent_->context(), index_, static_cast<int>(!muted_), PaSetMuteCb, data);
  if (!op) {
    delete data;
    picojson::object js_reply;

    js_reply["req_id"] = picojson::value(req_id);
    js_reply["error"] = picojson::value(true);
    js_reply["errorMsg"] = picojson::value(
        pa_strerror(pa_context_errno(parent_->context())));
    caller->PostMessage(js_reply);
    return;
  }

  // TODO(avalluri): push to pending queue
  pa_operation_unref(op);
}

void MuteControl::UpdateInfo(
    const pa_ext_volume_api_mute_control_info& info) {
  picojson::object js_message;
  picojson::array changes;

  if (label_ != info.description) {
    label_ = info.description;
    changes.push_back(picojson::value("label"));
  }

  if (muted_ != static_cast<bool>(info.mute)) {
    muted_ = static_cast<bool>(info.mute);
    changes.push_back(picojson::value("muted"));
  }

  js_message["cmd"] = picojson::value("mute_control_changed");
  js_message["control"] = picojson::value(ToJsonObject());
  js_message["changes"] = picojson::value(changes);
  parent_->PostMessage(js_message);
}

picojson::value::object MuteControl::ToJsonObject() const {
  picojson::value::object js_reply;

  js_reply["id"] = picojson::value(static_cast<double>(index_));
  js_reply["label"] = picojson::value(label_);
  js_reply["muted"] = picojson::value(muted_);
  return js_reply;
}

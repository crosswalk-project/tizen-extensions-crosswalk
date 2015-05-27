// Copyright(c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "audiosystem/audiosystem_context.h"

#ifndef NDEBUG
#include <sstream>
#endif  // NDEBUG
#include <vector>

#include "audiosystem/audiosystem_instance.h"
#include "audiosystem/audiosystem_logs.h"

AudioSystemContext::PendingQueue::~PendingQueue() {
  for (OperationMap::iterator it = queue_.begin(); it != queue_.end(); it++) {
    for (std::map<uint32_t, Operation*>::iterator
        op_it = it->second.begin(); op_it != it->second.end(); op_it++)
      delete op_it->second;
  }

  queue_.clear();
}

void AudioSystemContext::PendingQueue::Add(OperationType type,
    uint32_t index, pa_operation* op) {
  if (queue_[type][index])
    delete queue_[type][index];
  queue_[type][index] = new Operation(op);
}

void AudioSystemContext::PendingQueue::Remove(OperationType type,
    uint32_t index) {
  OperationMap::iterator it = queue_.find(type);
  if (it == queue_.end())
    return;

  std::map<uint32_t, Operation*>::iterator op_it = it->second.find(index);
  if (op_it == it->second.end())
    return;

  delete op_it->second;
  it->second.erase(op_it);
  if (it->second.empty())
    queue_.erase(it);
}

AudioSystemContext::Operation* AudioSystemContext::PendingQueue::Find(
    OperationType t, uint32_t index) const {
  OperationMap::const_iterator it = queue_.find(t);

  if (it == queue_.end())
    return NULL;

  std::map<uint32_t, Operation*>::const_iterator op_it =
      it->second.find(index);
  if (op_it == it->second.end())
    return NULL;

  return op_it->second;
}

bool AudioSystemContext::PendingQueue::Empty() const {
  return queue_.empty();
}

void AudioSystemContext::PendingQueue::Dump() const {
#ifndef NDEBUG
  const char* req[] = {
    "kGetServerInfo",
    "kGetVolumeControls",
    "kGetMuteControls",
    "kGetAudioGroups",
    "kGetAudioDevices",
    "kGetAudioStreams",
    "kGetVolumeControlInfo",
    "kRemoveVolumeControl",
    "kGetMuteControlInfo",
    "kRemoveMuteControl",
    "kGetAudioGroupInfo",
    "kRemoveAudioGroup",
    "kGetAudioDeviceInfo",
    "kRemoveAudioDevice",
    "kGetAudioStreamInfo",
    "kRemvoeAudioStream"
  };

  OperationMap::const_iterator it, end = queue_.end();
  std::ostringstream str;

  str << "Request Queue length :#" << queue_.size();

  for (it = queue_.begin(); it != end; it++) {
    str << "\nRequest Type: " << req[it->first];
    std::map<uint32_t, Operation*>::const_iterator op_it;
    for (op_it = it->second.begin(); op_it != it->second.end(); op_it++) {
      str << "\n  Request index  #" << op_it->first;
    }
  }

  DBG("%s", str.str().c_str());
#endif  // NDEBUG
}

#define PA_ERR(c, frmt_str, argc...) \
  ERR(frmt_str ", Error: '%s'", ##argc, pa_strerror(pa_context_errno(c)))

#define VALIDATE_PA_INFO(c, info, is_last) \
  if (is_last < 0 ||(is_last == 0 && !info)) {\
    PA_ERR(c, "Invalid pa info(%d, %p)", is_last, info);\
    return;\
  }

AudioSystemContext::AudioSystemContext()
    : context_(0),
      mainloop_(pa_mainloop_new()),
      is_ready_(false),
      main_output_volume_control_(PA_INVALID_INDEX),
      main_input_volume_control_(PA_INVALID_INDEX),
      main_output_mute_control_(PA_INVALID_INDEX),
      main_input_mute_control_(PA_INVALID_INDEX) {
}

AudioSystemContext::~AudioSystemContext() {
  DBG("destroy");
  Disconnect();
  pa_mainloop_free(mainloop_);
}

bool AudioSystemContext::IsConnected() const {
  return context_ && PA_CONTEXT_IS_GOOD(pa_context_get_state(context_));
}

bool AudioSystemContext::Connect() {
  // check if already connected
  if (IsConnected())
    return false;

  if (!context_) {
    pa_mainloop_api* api = pa_mainloop_get_api(mainloop_);
    context_ = pa_context_new_with_proplist(api, 0, 0);
    pa_context_set_state_callback(context_, PaStateChangeCb, this);
  }

  return pa_context_connect(context_, 0, PA_CONTEXT_NOFLAGS, 0) == 0;
}

bool AudioSystemContext::Disconnect() {
  if (context_) {
    pa_context_disconnect(context_);
    pa_context_set_state_callback(context_, 0, 0);
    pa_context_unref(context_);
    context_ = 0;
  }

  for (std::map<uint32_t, VolumeControl*>::iterator
      it = volume_controls_.begin(); it != volume_controls_.end(); it++)
    delete it->second;
  volume_controls_.clear();

  for (std::map<uint32_t, MuteControl*>::iterator
      it = mute_controls_.begin(); it != mute_controls_.end(); it++)
    delete it->second;
  mute_controls_.clear();

  for (std::map<uint32_t, AudioDevice*>::iterator
      it = audio_devices_.begin(); it != audio_devices_.end(); it++)
    delete it->second;
  audio_devices_.clear();

  for (std::map<uint32_t, AudioGroup*>::iterator
      it = audio_groups_.begin(); it != audio_groups_.end(); it++)
    delete it->second;
  audio_groups_.clear();

  for (std::map<uint32_t, AudioStream*>::iterator
      it = audio_streams_.begin(); it != audio_streams_.end(); it++)
    delete it->second;
  audio_streams_.clear();

  is_ready_ = false;
  main_input_mute_control_ = PA_INVALID_INDEX;
  main_output_mute_control_ = PA_INVALID_INDEX;
  return true;
}

//
// pulse audio context callbacks
//
void AudioSystemContext::PaStateChangeCb(pa_context* c, void* userdata) {
  reinterpret_cast<AudioSystemContext*>(userdata)->HandleStateChange();
}

void AudioSystemContext::PaVolumeApiStateChangeCb(
    pa_context* c, void* userdata) {
  AudioSystemContext* self = reinterpret_cast<AudioSystemContext*>(userdata);

  switch (pa_ext_volume_api_get_state(c)) {
    case PA_EXT_VOLUME_API_STATE_READY: {
      pa_ext_volume_api_set_subscribe_callback(c,
          PaVolumeApiSubscribeCb, userdata);
      pa_operation* op = pa_ext_volume_api_subscribe(c,
          PA_EXT_VOLUME_API_SUBSCRIPTION_MASK_ALL, 0, userdata);
      if (!op) {
        WARN("Failed to subscribe volume control changes");
      } else {
        pa_operation_unref(op);
      }

      self->UpdateVolumeControls();
      self->UpdateMuteControls();
      self->UpdateAudioGroups();
      self->UpdateAudioDevices();
      self->UpdateAudioStreams();
      self->UpdateServerInfo();
      break;
    }
    case PA_EXT_VOLUME_API_STATE_FAILED: {
      const char* err = pa_strerror(pa_context_errno(c));
      ERR("Failed to connect tizen volume control module : '%s'", err);
      self->EmitError(err);
      break;
    }
    default:
      break;
  }
}

void AudioSystemContext::PaVolumeApiServerInfoCb(
    pa_context* c, const pa_ext_volume_api_server_info* info, void* userdata) {
  VALIDATE_PA_INFO(c, info, 1);

  DBG("Received server info: \n"
      "vc_in: #%u, vc_out: #%u, mc_in: #%u, mc_out: #%u.",
      info->main_input_volume_control,
      info->main_output_volume_control,
      info->main_input_mute_control,
      info->main_output_mute_control);

  AudioSystemContext* self = reinterpret_cast<AudioSystemContext*>(userdata);
  picojson::value::object changes;

  if (self->main_input_volume_control_ != info->main_input_volume_control) {
    self->main_input_volume_control_ = info->main_input_volume_control;
    changes["main_input_volume_control"] = picojson::value(
        static_cast<double>(self->main_input_volume_control_));
  }

  if (self->main_output_volume_control_ != info->main_output_volume_control) {
    self->main_output_volume_control_ = info->main_output_volume_control;
    changes["main_output_volume_control"] = picojson::value(
        static_cast<double>(self->main_output_volume_control_));
  }

  if (self->main_input_mute_control_ != info->main_input_mute_control) {
    self->main_input_mute_control_ = info->main_input_mute_control;
    changes["main_input_mute_control"] = picojson::value(
        static_cast<double>(self->main_input_mute_control_));
  }

  if (self->main_output_mute_control_ != info->main_output_mute_control) {
    self->main_output_mute_control_ = info->main_output_mute_control;
    changes["main_output_mute_control"] = picojson::value(
        static_cast<double>(self->main_output_mute_control_));
  }

  if (self->is_ready_ && changes.size()) {
    picojson::object js_message;

    js_message["cmd"] = picojson::value("context_changed");
    js_message["changes"] = picojson::value(changes);

    self->PostMessage(js_message);
  }

  self->UnsetOperation(kGetServerInfo, 0);

  self->pending_queue_.Dump();
}

void AudioSystemContext::PaVolumeControlInfoCb(
    pa_context* c, const pa_ext_volume_api_volume_control_info* info,
    int eol, void* userdata) {
  VALIDATE_PA_INFO(c, info, eol);

  AudioSystemContext* self = reinterpret_cast<AudioSystemContext*>(userdata);

  eol == 1 ? self->UnsetOperation(kGetVolumeControls, 0)
           : self->HandleVolumeControlInfo(*info);
}

void AudioSystemContext::PaMuteControlInfoCb(
    pa_context* c, const pa_ext_volume_api_mute_control_info* info, int eol,
    void* userdata) {
  VALIDATE_PA_INFO(c, info, eol);

  AudioSystemContext* self = reinterpret_cast<AudioSystemContext*>(userdata);

  eol == 1 ? self->UnsetOperation(kGetMuteControls, 0)
           : self->HandleMuteControlInfo(*info);
}

void AudioSystemContext::PaAudioDeviceInfoCb(
    pa_context* c, const pa_ext_volume_api_device_info* info,
    int eol, void* userdata) {
  VALIDATE_PA_INFO(c, info, eol);

  AudioSystemContext* self = reinterpret_cast<AudioSystemContext*>(userdata);

  eol == 1 ? self->UnsetOperation(kGetAudioDevices, 0)
           : self->HandleAudioDeviceInfo(*info);
}

void AudioSystemContext::PaAudioGroupInfoCb(
    pa_context* c, const pa_ext_volume_api_audio_group_info* info,
    int eol, void* userdata) {
  VALIDATE_PA_INFO(c, info, eol);

  AudioSystemContext* self = reinterpret_cast<AudioSystemContext*>(userdata);

  eol == 1 ? self->UnsetOperation(kGetAudioGroups, 0)
           : self->HandleAudioGroupInfo(*info);
}

void AudioSystemContext::PaAudioStreamInfoCb(
    pa_context* c, const pa_ext_volume_api_stream_info* info, int eol,
    void* userdata) {
  VALIDATE_PA_INFO(c, info, eol);

  AudioSystemContext* self = reinterpret_cast<AudioSystemContext*>(userdata);

  eol == 1 ? self->UnsetOperation(kGetAudioStreams, 0)
           : self->HandleAudioStreamInfo(*info);
}

void AudioSystemContext::PaVolumeApiSubscribeCb(
    pa_context* c, pa_ext_volume_api_subscription_event_type t,
    uint32_t index, void* userdata) {
  AudioSystemContext* self = reinterpret_cast<AudioSystemContext*>(userdata);

  self->HandleVolumeApiEvent(t, index);
}

//
//  private helper methods
//
bool AudioSystemContext::UpdateVolumeControls() {
  pa_operation* op = pa_ext_volume_api_get_volume_control_info_list(
      context_, PaVolumeControlInfoCb, this);
  if (!op) {
    PA_ERR(context_, "Failed to get volume control info list");
    return false;
  }

  pending_queue_.Add(kGetVolumeControls, 0, op);

  return true;
}

bool AudioSystemContext::UpdateMuteControls() {
  pa_operation* op = pa_ext_volume_api_get_mute_control_info_list(
      context_, PaMuteControlInfoCb, this);
  if (!op) {
    PA_ERR(context_, "Failed to get mute control info list");
    return false;
  }

  pending_queue_.Add(kGetMuteControls, 0, op);

  return true;
}

bool AudioSystemContext::UpdateAudioGroups() {
  pa_operation* op = pa_ext_volume_api_get_audio_group_info_list(
      context_, PaAudioGroupInfoCb, this);
  if (!op) {
    PA_ERR(context_, "Failed to get audio group info list");
    return false;
  }

  pending_queue_.Add(kGetAudioGroups, 0, op);
  return true;
}

bool AudioSystemContext::UpdateAudioDevices() {
  pa_operation* op = pa_ext_volume_api_get_device_info_list(
      context_, PaAudioDeviceInfoCb, this);
  if (!op) {
    PA_ERR(context_, "Failed to get audio device info list");
    return false;
  }

  pending_queue_.Add(kGetAudioDevices, 0, op);
  return true;
}

bool AudioSystemContext::UpdateAudioStreams() {
  pa_operation* op = pa_ext_volume_api_get_stream_info_list(
      context_, PaAudioStreamInfoCb, this);
  if (!op) {
    PA_ERR(context_, "Failed to get audio stream info list");
    return false;
  }

  pending_queue_.Add(kGetAudioStreams, 0, op);
  return true;
}

bool AudioSystemContext::UpdateServerInfo() {
  Operation* o = pending_queue_.Find(kGetServerInfo, 0);
  // Ignore if previous server info is pending
  // It's safe check, but in reality non-running operaitons
  // shouldn't exist in our cached map.
  if (o && o->state() == PA_OPERATION_RUNNING)
    return true;

  DBG("Requesting for new info...");

  // get server info, to know default sink/source
  pa_operation* op = pa_ext_volume_api_get_server_info(
      context_, PaVolumeApiServerInfoCb, this);
  if (!op) {
    PA_ERR(context_, "Failed to get server info");
    return false;
  }

  pending_queue_.Add(kGetServerInfo, 0, op);
  return true;
}

bool AudioSystemContext::UpdateVolumeControl(uint32_t index) {
  // ignore if previous request in progress
  if (pending_queue_.Find(kGetVolumeControlInfo, index))
    return true;

  DBG("Getting volume control info for '%u", index);
  pa_operation* op = pa_ext_volume_api_get_volume_control_info_by_index(
      context_, index, PaVolumeControlInfoCb, this);
  if (!op) {
    PA_ERR(context_, "Failed to get volume control info for index '%u'",
        index);
    return false;
  }

  pending_queue_.Add(kGetVolumeControlInfo, index, op);
  return true;
}

bool AudioSystemContext::UpdateMuteControl(uint32_t index) {
  if (pending_queue_.Find(kGetMuteControlInfo, index)) {
    return true;
  }

  DBG("Getting mute control info for '%u", index);
  pa_operation* op = pa_ext_volume_api_get_mute_control_info_by_index(
      context_, index, PaMuteControlInfoCb, this);
  if (!op) {
    PA_ERR(context_, "Failed to get mute control info for index '%u'",
        index);
    return false;
  }

  pending_queue_.Add(kGetMuteControlInfo, index, op);
  return true;
}

bool AudioSystemContext::UpdateAudioDevice(uint32_t index) {
  if (pending_queue_.Find(kGetAudioDeviceInfo, index)) {
    return true;
  }

  DBG("Getting audio device info for '%u", index);
  pa_operation* op = pa_ext_volume_api_get_device_info_by_index(
      context_, index, PaAudioDeviceInfoCb, this);
  if (!op) {
    PA_ERR(context_, "Failed to get audio device info for index '%u'",
        index);
    return false;
  }

  pending_queue_.Add(kGetAudioDeviceInfo, index, op);
  return true;
}

bool AudioSystemContext::UpdateAudioGroup(uint32_t index) {
  if (pending_queue_.Find(kGetAudioGroupInfo, index)) {
    return true;
  }

  DBG("Getting audio group info for '%u", index);
  pa_operation* op = pa_ext_volume_api_get_audio_group_info_by_index(
      context_, index, PaAudioGroupInfoCb, this);
  if (!op) {
    PA_ERR(context_, "Failed to get audio group info for index '%u'",
        index);
    return false;
  }

  pending_queue_.Add(kGetAudioGroupInfo, index, op);
  return true;
}

bool AudioSystemContext::UpdateAudioStream(uint32_t index) {
  if (pending_queue_.Find(kGetAudioStreamInfo, index)) {
    return true;
  }

  DBG("Getting audio stream info for '%u", index);
  pa_operation* op = pa_ext_volume_api_get_stream_info_by_index(
      context_, index, PaAudioStreamInfoCb, this);
  if (!op) {
    PA_ERR(context_, "Failed to get audio stream info for index '%u'",
        index);
    return false;
  }

  pending_queue_.Add(kGetAudioStreamInfo, index, op);
  return true;
}

void AudioSystemContext::HandleStateChange() {
  pa_context_state_t state = pa_context_get_state(context_);

  switch (state) {
    case PA_CONTEXT_UNCONNECTED:
      DBG("UNCONNECTED"); break;
    case PA_CONTEXT_CONNECTING:
      DBG("CONNECTING"); break;
    case PA_CONTEXT_AUTHORIZING:
      DBG("AUTHORIZING"); break;
    case PA_CONTEXT_SETTING_NAME:
      DBG("SETTING NAME"); break;
    case PA_CONTEXT_READY:
      DBG("READY");
      pa_ext_volume_api_set_state_callback(
          context_, PaVolumeApiStateChangeCb, this);

      pa_ext_volume_api_connect(context_);
      break;
    case PA_CONTEXT_FAILED:
      PA_ERR(context_, "FAILED to connect.");
      EmitError(pa_strerror(pa_context_errno(context_)));
      break;
    case PA_CONTEXT_TERMINATED:
      DBG("TERMINATED");
      pa_ext_volume_api_disconnect(context_);
      pa_ext_volume_api_set_state_callback(context_, 0, 0);
      pa_context_set_subscribe_callback(context_, 0, 0);
      EmitDisconnected();
      break;
    default:
      break;
  }
}

void AudioSystemContext::HandleVolumeApiEvent(
    pa_ext_volume_api_subscription_event_type_t event, uint32_t index) {
  bool removed = (event & PA_SUBSCRIPTION_EVENT_TYPE_MASK)
      == PA_SUBSCRIPTION_EVENT_REMOVE;

  switch (event & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
    case PA_EXT_VOLUME_API_SUBSCRIPTION_EVENT_VOLUME_CONTROL:
      if (removed) {
        delete volume_controls_[index];
        volume_controls_.erase(index);
        EmitVolumeControlRemoved(index);
      } else {
        UpdateVolumeControl(index);
      }
      break;
    case PA_EXT_VOLUME_API_SUBSCRIPTION_EVENT_MUTE_CONTROL:
      if (event & PA_SUBSCRIPTION_EVENT_REMOVE) {
        delete mute_controls_[index];
        mute_controls_.erase(index);
        EmitMuteControlRemoved(index);
      } else {
        UpdateMuteControl(index);
      }
      break;
    case PA_EXT_VOLUME_API_SUBSCRIPTION_EVENT_DEVICE:
      if (removed) {
        delete audio_devices_[index];
        audio_devices_.erase(index);
        EmitAudioDeviceRemoved(index);
      } else {
        UpdateAudioDevice(index);
      }
      break;
    case PA_EXT_VOLUME_API_SUBSCRIPTION_EVENT_STREAM:
      if (removed) {
        delete audio_streams_[index];
        audio_streams_.erase(index);
        EmitAudioStreamRemoved(index);
      } else {
        UpdateAudioStream(index);
      }
      break;
    case PA_EXT_VOLUME_API_SUBSCRIPTION_EVENT_AUDIO_GROUP:
      if (removed) {
        delete audio_groups_[index];
        audio_groups_.erase(index);
        EmitAudioGroupRemoved(index);
      } else {
        UpdateAudioGroup(index);
      }
      break;
    case PA_EXT_VOLUME_API_SUBSCRIPTION_EVENT_SERVER:
      UpdateServerInfo();
      break;
    default:
      DBG("Unknown event '%x', ignoring!!!", event);
  }
}

void AudioSystemContext::HandleVolumeControlInfo(
    const pa_ext_volume_api_volume_control_info& info) {
  if (!volume_controls_.count(info.index)) {
    DBG("New VolumeControl #%u: %s", info.index, info.name);
    VolumeControl* vc = new VolumeControl(this, info);
    volume_controls_[info.index] = vc;
    // Emit signals only after ready signal
    if (is_ready_) EmitVolumeControlAdded(*vc);
  } else {
    DBG("Updated VolumeControl #%u: %s", info.index, info.name);
    VolumeControl* vc = volume_controls_[info.index];
    vc->UpdateInfo(info);
  }

  UnsetOperation(kGetVolumeControlInfo, info.index);
}

void AudioSystemContext::HandleMuteControlInfo(
    const pa_ext_volume_api_mute_control_info& info) {
  if (!mute_controls_.count(info.index)) {
    DBG("New MuteControl : #%u %s", info.index, info.name);
    MuteControl* mc = new MuteControl(this, info);
    mute_controls_[info.index] = mc;
    if (is_ready_) EmitMuteControlAdded(*mc);
  } else {
    DBG("Updated MuteControl : #%u %s", info.index, info.name);
    MuteControl* mc = mute_controls_[info.index];
    mc->UpdateInfo(info);
  }

  UnsetOperation(kGetMuteControlInfo, info.index);
}

void AudioSystemContext::HandleAudioDeviceInfo(
    const pa_ext_volume_api_device_info& info) {
  if (!audio_devices_.count(info.index)) {
    DBG("New AudioDevice : #%u %s", info.index, info.name);
    AudioDevice* ad = new AudioDevice(this, info);
    audio_devices_[info.index] = ad;
    if (is_ready_) EmitAudioDeviceAdded(*ad);
  } else {
    DBG("Updated AudioDevice : #%u %s", info.index, info.name);
    AudioDevice* ad = audio_devices_[info.index];
    ad->UpdateInfo(info);
  }

  UnsetOperation(kGetAudioDeviceInfo, info.index);
}

void AudioSystemContext::HandleAudioGroupInfo(
    const pa_ext_volume_api_audio_group_info& info) {
  if (!audio_groups_.count(info.index)) {
    DBG("New AudioGroup #%u: %s", info.index, info.name);
    AudioGroup* ag = new AudioGroup(this, info);
    audio_groups_[info.index] = ag;
    // Emit signals only after ready signal
    if (is_ready_) EmitAudioGroupAdded(*ag);
  } else {
    DBG("Updated AudioGroup #%u: %s", info.index, info.name);
    AudioGroup* ag = audio_groups_[info.index];
    ag->UpdateInfo(info);
  }

  UnsetOperation(kGetAudioGroupInfo, info.index);
}

void AudioSystemContext::HandleAudioStreamInfo(
    const pa_ext_volume_api_stream_info& info) {
  if (!audio_streams_.count(info.index)) {
    DBG("New AudioStream : #%u %s", info.index, info.name);
    AudioStream* as = new AudioStream(this, info);
    audio_streams_[info.index] = as;
    // Emit signals only after ready signal
    if (is_ready_) EmitAudioStreamAdded(*as);
  } else {
    DBG("Updated AudioStream : #%u %s", info.index, info.name);
    AudioStream* as = audio_streams_[info.index];
    as->UpdateInfo(info);
  }

  UnsetOperation(kGetAudioStreamInfo, info.index);
}

void AudioSystemContext::UnsetOperation(OperationType type, uint32_t index) {
  pending_queue_.Remove(type, index);

  if (pending_queue_.Empty())
    is_ready_ ? EmitSync() : EmitReady();
}

void AudioSystemContext::EmitSync() {
  picojson::value::object js_reply;

  js_reply["cmd"] = picojson::value("sync");
  PostMessage(js_reply);
}

void AudioSystemContext::EmitReady() {
  picojson::value::object js_reply;

  js_reply["context"] = picojson::value(ToJsonObject());
  PostMessageWithReqId(js_reply);
  is_ready_ = true;
}

void AudioSystemContext::EmitError(const std::string& err) {
  picojson::value::object js_reply;

  js_reply["error"] = picojson::value(true);
  js_reply["errorMsg"] = picojson::value(err);
  PostMessageWithReqId(js_reply);
}

void AudioSystemContext::EmitDisconnected() {
  DBG("Postmessage : disconnected");
  picojson::value::object js_reply;

  js_reply["cmd"] = picojson::value("disconnected");
  PostMessage(js_reply);
}

void AudioSystemContext::EmitVolumeControlAdded(const VolumeControl& vc) {
  DBG("volumecontroladded");
  picojson::value::object js_message;

  js_message["cmd"] = picojson::value("volume_control_added");
  js_message["control"] = picojson::value(vc.ToJsonObject());
  PostMessage(js_message);
}

void AudioSystemContext::EmitVolumeControlRemoved(uint32_t index) {
  DBG("volumecontrolremoved");
  picojson::value::object js_message;

  js_message["cmd"] = picojson::value("volume_control_removed");
  js_message["id"] = picojson::value(static_cast<double>(index));
  PostMessage(js_message);
}

void AudioSystemContext::EmitMuteControlAdded(const MuteControl& mc) {
  DBG("mutecontroladded");
  picojson::value::object js_message;

  js_message["cmd"] = picojson::value("mute_control_added");
  js_message["control"] = picojson::value(mc.ToJsonObject());
  PostMessage(js_message);
}

void AudioSystemContext::EmitMuteControlRemoved(uint32_t index) {
  DBG("mutecontrolremoved");
  picojson::value::object js_message;

  js_message["cmd"] = picojson::value("mute_control_removed");
  js_message["id"] = picojson::value(static_cast<double>(index));
  PostMessage(js_message);
}

void AudioSystemContext::EmitAudioGroupAdded(const AudioGroup& ag) {
  DBG("audiogroupadded");
  picojson::value::object js_message;

  js_message["cmd"] = picojson::value("group_added");
  js_message["group"] = picojson::value(ag.ToJsonObject());
  PostMessage(js_message);
}

void AudioSystemContext::EmitAudioGroupRemoved(uint32_t index) {
  DBG("audiogroupremoved");
  picojson::value::object js_message;

  js_message["cmd"] = picojson::value("group_removed");
  js_message["id"] = picojson::value(static_cast<double>(index));
  PostMessage(js_message);
}

void AudioSystemContext::EmitAudioStreamAdded(const AudioStream& as) {
  DBG("audiostreamadded");
  picojson::value::object js_message;

  js_message["cmd"] = picojson::value("stream_added");
  js_message["stream"] = picojson::value(as.ToJsonObject());
  PostMessage(js_message);
}

void AudioSystemContext::EmitAudioStreamRemoved(uint32_t index) {
  DBG("audiostreamremoved");
  picojson::value::object js_message;

  js_message["cmd"] = picojson::value("stream_removed");
  js_message["id"] = picojson::value(static_cast<double>(index));
  PostMessage(js_message);
}

void AudioSystemContext::EmitAudioDeviceAdded(const AudioDevice& ad) {
  DBG("audiodeviceadded");
  picojson::value::object js_message;

  js_message["cmd"] = picojson::value("device_added");
  js_message["device"] = picojson::value(ad.ToJsonObject());
  PostMessage(js_message);
}

void AudioSystemContext::EmitAudioDeviceRemoved(uint32_t index) {
  DBG("audiodeviceremoved");
  picojson::value::object js_message;

  js_message["cmd"] = picojson::value("device_removed");
  js_message["id"] = picojson::value(static_cast<double>(index));
  PostMessage(js_message);
}

picojson::value::object AudioSystemContext::ToJsonObject() const {
  picojson::value::object reply;

  reply["volume_controls"] = picojson::value(
      MapToJSArray<VolumeControl*>(volume_controls_));
  reply["mute_controls"] = picojson::value(
      MapToJSArray<MuteControl*>(mute_controls_));
  reply["devices"] = picojson::value(
      MapToJSArray<AudioDevice*>(audio_devices_));
  reply["streams"] = picojson::value(
      MapToJSArray<AudioStream*>(audio_streams_));
  reply["audio_groups"] = picojson::value(
      MapToJSArray<AudioGroup*>(audio_groups_));

  if (main_output_volume_control_ != PA_INVALID_INDEX) {
    reply["main_output_volume_control"] = picojson::value(
        static_cast<double>(main_output_volume_control_));
  }

  if (main_input_volume_control_ != PA_INVALID_INDEX) {
    reply["main_input_volume_control"] = picojson::value(
        static_cast<double>(main_input_volume_control_));
  }

  if (main_output_mute_control_ != PA_INVALID_INDEX) {
    reply["main_output_mute_control"] = picojson::value(
        static_cast<double>(main_output_mute_control_));
  }

  if (main_input_volume_control_ != PA_INVALID_INDEX) {
    reply["main_input_mute_control"] = picojson::value(
        static_cast<double>(main_input_mute_control_));
  }

  return reply;
}

void AudioSystemContext::PostMessage(const picojson::object& js_message) const {
  std::map<AudioSystemInstance*, std::string>::const_iterator it;

  for (it = listeners_.begin(); it != listeners_.end(); it++)
    it->first->PostMessage(js_message);
}

void AudioSystemContext::PostMessageWithReqId(
    const picojson::object& js_message) const {
  picojson::value::object msg = js_message;
  std::map<AudioSystemInstance*, std::string>::const_iterator it;

  for (it = listeners_.begin(); it != listeners_.end(); it++) {
    msg["req_id"] = picojson::value(it->second);
    it->first->PostMessage(msg);
  }
}

void AudioSystemContext::HandleMessage(const picojson::value& js_message,
    AudioSystemInstance* caller) {
  const std::string& command = js_message.get("cmd").to_str();
  const std::string& req_id  = js_message.get("req_id").to_str();

  picojson::value::object js_reply;
  js_reply["req_id"] = picojson::value(req_id);

  if (command == "connect") {
    listeners_[caller] = req_id;
    if (!IsConnected()) {
      DBG("Got Connect request, id : %s",
          js_message.get("req_id").to_str().c_str());
      Connect();
    } else if (is_ready_) {
      // Connection is ready send the context info
      js_reply["context"] = picojson::value(ToJsonObject());

      caller->PostMessage(js_reply);
    } else {
      // Do nothing, as connection in prgress, once its ready
      // caller will be infomed via EmitReady() signal,
    }
  } else if (command == "disconnect") {
    std::map<AudioSystemInstance*, std::string>::iterator it =
        listeners_.find(caller);
    if (it != listeners_.end())
      listeners_.erase(it);

    // treat as if its disconnected
    caller->PostMessage(js_reply);

    // send dummy 'disconnected' signal to caller,
    // as caller may expect this signal, when disconnected
    picojson::value::object js_message;
    js_message["cmd"] = picojson::value("disconnected");
    caller->PostMessage(js_message);

    // Real disconnection from server : only when there
    // are no active clients
    if (listeners_.size() == 0 && IsConnected()) {
      DBG("Disconnecting from server");
      Disconnect();
    }
  } else if (command == "setMuted") {
    try {
      picojson::value::object js_reply;
      uint32_t index = static_cast<uint32_t>(
          js_message.get("id").get<double>());
      bool muted = js_message.get("muted").get<bool>();
      MuteControl* mc = mute_controls_[index];

      DBG("Changing mute of control '%u' to '%d'", index, muted);
      if (!mc) {
        js_reply["error"] = picojson::value(true);
        js_reply["errorMsg"] = picojson::value(
            "No mute control found for given id");

        caller->PostMessage(js_reply);
      } else if (muted == mc->muted()) {
        caller->PostMessage(js_reply);
      } else {
        mc->ToggleMute(caller, req_id);
      }
    } catch(std::overflow_error& exp) {
      ERR("Exception : %s", exp.what());
    }
  } else if (command == "setVolume") {
    try {
      uint32_t index = static_cast<uint32_t>(
          js_message.get("id").get<double>());
      double vol = js_message.get("volume").get<double>();
      DBG("Setting Volume of '%u' to '%.2f'", index, vol);
      VolumeControl* vc = volume_controls_[index];
      if (!vc) {
        picojson::value::object js_reply;

        js_reply["error"] = picojson::value(true);
        js_reply["errorMsg"] = picojson::value(
            "No volume control found with given id");

        caller->PostMessage(js_reply);
      } else {
        vc->SetVolume(vol, caller, req_id);
      }
    } catch(std::overflow_error& exp) {
      ERR("Exception : %s", exp.what());
    }
  } else if (command == "setBalance") {
    try {
      DBG("setBalance");
      uint32_t index = static_cast<uint32_t>(
          js_message.get("id").get<double>());
      VolumeControl* vc = volume_controls_[index];

      if (!vc) {
        picojson::value::object js_reply;

        js_reply["error"] = picojson::value(true);
        js_reply["errorMsg"] = picojson::value(
            "No volume control found for given id");

        caller->PostMessage(js_reply);
      } else {
        std::vector<double> balance;
        const picojson::array balance_array =
          js_message.get("balance").get<picojson::array>();

        for (int i = 0; i < balance_array.size(); i++) {
          balance.push_back(balance_array.at(i).get<double>());
        }

        vc->SetBalance(balance, caller, req_id);
      }
    } catch(std::overflow_error& exp) {
      ERR("Exception: %s", exp.what());
    }
  } else if (command == "setSimplifiedBalance") {
    DBG("setSimplifiedBalance");
    uint32_t index = static_cast<uint32_t>(js_message.get("id").get<double>());
    VolumeControl* vc = volume_controls_[index];

    if (!vc) {
      js_reply["error"] = picojson::value(true);
      js_reply["errorMsg"] = picojson::value(
          "No volume control found with given id");

      caller->PostMessage(js_reply);
    } else {
      double balance = js_message.get("balance").get<double>();
      double fade = js_message.get("fade").get<double>();

      vc->SetSimplifiedBalance(balance, fade, caller, req_id);
    }
  } else {
    std::string err = "Unknown command '" + command + "'";
    js_reply["error"] = picojson::value(true);
    js_reply["errorMsg"] = picojson::value(err);

    caller->PostMessage(js_reply);
  }
}

void AudioSystemContext::SendSyncMessage(
    const picojson::value::object& js_message) {
  std::map<AudioSystemInstance*, std::string>::const_iterator it;

  for (it = listeners_.begin(); it != listeners_.end(); it++) {
    it->first->SendSyncReply(js_message);
  }
}

picojson::value::object AudioSystemContext::HandleSyncMessage(
    const picojson::value& js_message) {
  picojson::value::object js_reply;

  std::string command = js_message.get("cmd").to_str();

  if (command == "getSimplifiedBalance") {
    uint32_t index = static_cast<uint32_t>(js_message.get("id").get<double>());
    VolumeControl* vc = volume_controls_[index];
    if (!vc) {
      js_reply["error"] = picojson::value(true);
      js_reply["errorMsg"] = picojson::value(
          "No volume control found with given id");
    } else {
      return vc->GetSimplifiedBalance();
    }
  } else {
    js_reply["error"] = picojson::value(true);
    js_reply["errorMsg"] = picojson::value(
        "Unknown command sync command");
  }

  return js_reply;
}

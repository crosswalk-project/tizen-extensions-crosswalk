// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AUDIOSYSTEM_AUDIOSYSTEM_CONTEXT_H_
#define AUDIOSYSTEM_AUDIOSYSTEM_CONTEXT_H_

#include <pulse/ext-volume-api.h>
#include <pulse/mainloop.h>
#include <pulse/pulseaudio.h>
#include <map>
#include <string>

#include "audiosystem/audiosystem_audio_group.h"
#include "audiosystem/audiosystem_device.h"
#include "audiosystem/audiosystem_mute_control.h"
#include "audiosystem/audiosystem_stream.h"
#include "audiosystem/audiosystem_volume_control.h"
#include "common/picojson.h"
#include "common/utils.h"

class AudioSystemInstance;

class AudioSystemContext {
  enum OperationType {
    kGetServerInfo = 0,
    kGetVolumeControls,
    kGetMuteControls,
    kGetAudioGroups,
    kGetAudioDevices,
    kGetAudioStreams,
    kGetVolumeControlInfo,
    kRemoveVolumeControl,
    kGetMuteControlInfo,
    kRemoveMuteControl,
    kGetAudioGroupInfo,
    kRemoveAudioGroup,
    kGetAudioDeviceInfo,
    kRemoveAudioDevice,
    kGetAudioStreamInfo,
    kRemvoeAudioStream
  };

 public:
  class Operation {
   public:
    explicit Operation(pa_operation* op):op_(op) { }
    ~Operation() {
      if (pa_operation_get_state(op_) == PA_OPERATION_RUNNING)
        pa_operation_cancel(op_);
      pa_operation_unref(op_);
    }

    pa_operation* op() const { return op_; }
    pa_operation_state_t state() const { return pa_operation_get_state(op_); }

   private:
    pa_operation* op_;

    DISALLOW_COPY_AND_ASSIGN(Operation);
  };

  class PendingQueue {
    typedef std::map <OperationType,
                      std::map<uint32_t, Operation*> > OperationMap;
   public:
    PendingQueue() {}
    ~PendingQueue();

    void Add(OperationType type, uint32_t index, pa_operation* op);
    void Remove(OperationType type, uint32_t index);
    AudioSystemContext::Operation* Find(OperationType t, uint32_t index) const;
    bool Empty() const;
    void Dump() const;

   private:
    OperationMap queue_;

    DISALLOW_COPY_AND_ASSIGN(PendingQueue);
  };

  AudioSystemContext();
  ~AudioSystemContext();

  int Prepare() {
    return pa_mainloop_prepare(mainloop_, -1);
  }

  int Poll() {
    return pa_mainloop_poll(mainloop_);
  }

  int Dispatch() {
    return pa_mainloop_dispatch(mainloop_);
  }

  void QuitLoop(void) {
    pa_mainloop_quit(mainloop_, 0);
  }

  picojson::object ToJsonObject() const;
  void HandleMessage(const picojson::value& js_message,
                     AudioSystemInstance* caller);
  picojson::value::object HandleSyncMessage(const picojson::value& js_message);
  void PostMessage(const picojson::object& js_message) const;
  void PostMessageWithReqId(const picojson::object& js_message) const;
  void SendSyncMessage(const picojson::value::object& js_message);

  bool Connect();
  bool Disconnect();
  bool IsConnected() const;
  pa_context* context() const { return context_; }

 private:
  void UnsetOperation(OperationType type, uint32_t index);
  void HandleStateChange();
  void HandleVolumeControlInfo(
      const pa_ext_volume_api_volume_control_info& info);
  void HandleMuteControlInfo(
      const pa_ext_volume_api_mute_control_info& info);
  void HandleAudioGroupInfo(
      const pa_ext_volume_api_audio_group_info& info);
  void HandleAudioDeviceInfo(
      const pa_ext_volume_api_device_info& info);
  void HandleAudioStreamInfo(
      const pa_ext_volume_api_stream_info& info);
  void HandleVolumeApiEvent(
      pa_ext_volume_api_subscription_event_type_t event, uint32_t index);
  void HandleEvent(pa_subscription_event_type_t event, uint32_t index);

  bool UpdateServerInfo();
  bool UpdateVolumeControls();
  bool UpdateVolumeControl(uint32_t index);
  bool UpdateMuteControls();
  bool UpdateMuteControl(uint32_t index);
  bool UpdateAudioGroups();
  bool UpdateAudioGroup(uint32_t index);
  bool UpdateAudioDevices();
  bool UpdateAudioDevice(uint32_t index);
  bool UpdateAudioStreams();
  bool UpdateAudioStream(uint32_t index);

  void EmitSync();
  void EmitReady();
  void EmitError(const std::string& error);
  void EmitDisconnected();
  void EmitVolumeControlAdded(const VolumeControl& vc);
  void EmitVolumeControlRemoved(uint32_t index);
  void EmitMuteControlAdded(const MuteControl& mc);
  void EmitMuteControlRemoved(uint32_t index);
  void EmitAudioGroupAdded(const AudioGroup& ag);
  void EmitAudioGroupRemoved(uint32_t index);
  void EmitAudioDeviceAdded(const AudioDevice& ad);
  void EmitAudioDeviceRemoved(uint32_t index);
  void EmitAudioStreamAdded(const AudioStream& as);
  void EmitAudioStreamRemoved(uint32_t index);

  static void PaStateChangeCb(pa_context* c, void* userdata);
  static void PaVolumeApiSubscribeCb(pa_context* c,
      pa_ext_volume_api_subscription_event_type event,
      uint32_t index, void* userdata);

  static void PaVolumeApiStateChangeCb(pa_context* c, void* userdata);
  static void PaVolumeApiServerInfoCb(pa_context* c,
      const pa_ext_volume_api_server_info* info, void* userdata);
  static void PaVolumeControlInfoCb(pa_context* c,
      const pa_ext_volume_api_volume_control_info* info, int eol,
      void* userdata);
  static void PaMuteControlInfoCb(pa_context* c,
      const pa_ext_volume_api_mute_control_info* info, int eol, void* userdata);
  static void PaAudioDeviceInfoCb(pa_context* c,
      const pa_ext_volume_api_device_info* info, int eol, void* userdata);
  static void PaAudioGroupInfoCb(pa_context* c,
      const pa_ext_volume_api_audio_group_info* info, int eol, void* userdata);
  static void PaAudioStreamInfoCb(pa_context* c,
      const pa_ext_volume_api_stream_info* info, int eol, void* userdata);

 private:
  pa_context* context_;
  pa_mainloop* mainloop_;
  bool is_ready_;
  std::map<AudioSystemInstance*, std::string> listeners_;
  std::map<uint32_t, VolumeControl*> volume_controls_;
  std::map<uint32_t, MuteControl*> mute_controls_;
  std::map<uint32_t, AudioGroup*> audio_groups_;
  std::map<uint32_t, AudioDevice*> audio_devices_;
  std::map<uint32_t, AudioStream*> audio_streams_;
  uint32_t main_output_volume_control_;
  uint32_t main_input_volume_control_;
  uint32_t main_output_mute_control_;
  uint32_t main_input_mute_control_;
  PendingQueue pending_queue_;

  DISALLOW_COPY_AND_ASSIGN(AudioSystemContext);
};

template <class T>
picojson::array MapToJSArray(const std::map<uint32_t, T>& map) {
  picojson::array jsArray;

  typename std::map<uint32_t, T>::const_iterator it;
  for (it = map.begin(); it != map.end(); ++it) {
    jsArray.push_back(picojson::value(it->second->ToJsonObject()));
  }

  return jsArray;
}

#endif  // AUDIOSYSTEM_AUDIOSYSTEM_CONTEXT_H_

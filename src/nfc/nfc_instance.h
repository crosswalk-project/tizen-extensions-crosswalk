// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NFC_NFC_INSTANCE_H_
#define NFC_NFC_INSTANCE_H_

#include <nfc.h>
#include <string>
#include "common/extension.h"
#include "nfc/nfc_callbacks.h"

namespace picojson {

class value;

}  // namespace picojson

class NfcInstance : public common::Instance {
 public:
  NfcInstance();
  virtual ~NfcInstance();

 private:
  // common::Instance implementation.
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* msg);

  void PollStart(const picojson::value& value);
  void PollStop(const picojson::value& value);
  void ReadNdef(const picojson::value& value);
  void WriteNdef(const picojson::value& value);
  void SendNdef(const picojson::value& value);
  void StartHandover(const picojson::value& value);
  void GetBytes(const picojson::value& value);
  void GetPayload(const picojson::value& value);

  void SwitchPower(bool state, const picojson::value& value);
  void PostError(double async_operation_id);
  void PostSuccess(const std::string& event, double async_operation_id = 0);
  void PostResult(const std::string& command, double async_operation_id);
  void PostResult(const std::string& command, const picojson::value& value,
      double async_operation_id);

  CALLBACK_METHOD(OnActivationChanged, bool, NfcInstance);
  CALLBACK_METHOD_2(OnTagDiscovered, nfc_discovered_type_e,
      nfc_tag_h, NfcInstance);
  CALLBACK_METHOD_2(OnP2PDiscovered, nfc_discovered_type_e,
      nfc_p2p_target_h, NfcInstance);
  CALLBACK_METHOD_WITH_DATA(OnPowered, nfc_error_e, NfcInstance);
  CALLBACK_METHOD_WITH_DATA_2(OnReadNdef, nfc_error_e,
      nfc_ndef_message_h, NfcInstance);
  CALLBACK_METHOD_WITH_DATA(OnWriteNdef, nfc_error_e, NfcInstance);
  CALLBACK_METHOD_WITH_DATA_2(OnP2PDataRecieved, nfc_p2p_target_h,
      nfc_ndef_message_h, NfcInstance);
  CALLBACK_METHOD_WITH_DATA(OnP2PMessageSent, nfc_error_e, NfcInstance);

 private:
  int nfc_manager_init_code_ = 0;
  bool nfc_manager_activated_ = false;
  nfc_tag_h attached_tag_ = NULL;
  nfc_p2p_target_h connected_peer_ = NULL;
};

#endif  // NFC_NFC_INSTANCE_H_

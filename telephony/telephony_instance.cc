// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "telephony/telephony_instance.h"

#include <string>

#include "common/picojson.h"
#include "telephony/telephony_backend_ofono.h"
#include "tizen/tizen.h"

namespace {

const char kCmdGetServices[] = "getServices";
const char kCmdSetDefaultService[] = "setDefaultService";
const char kCmdSetServiceEnabled[] = "setServiceEnabled";
const char kCmdGetCalls[] = "getCalls";
const char kCmdDial[] = "dial";
const char kCmdAccept[] = "accept";
const char kCmdDisconnect[] = "disconnect";
const char kCmdHold[] = "hold";
const char kCmdResume[] = "resume";
const char kCmdDeflect[] = "deflect";
const char kCmdTransfer[] = "transfer";
const char kCmdSplit[] = "split";
const char kCmdSendTones[] = "sendTones";
const char kCmdStartTone[] = "startTone";
const char kCmdStopTone[] = "stopTone";
const char kCmdGetEmergencyNumbers[] = "getEmergencyNumbers";
const char kCmdEmergencyDial[] = "emergencyDial";
const char kCmdCreateConference[] = "createConference";
const char kCmdGetParticipants[] = "getParticipants";
const char kCmdEnableNotifications[] = "enableNotifications";
const char kCmdDisableNotifications[] = "disableNotifications";

}  // namespace

TelephonyInstance::TelephonyInstance() : backend_(new TelephonyBackend(this)) {
}

TelephonyInstance::~TelephonyInstance() {
  delete backend_;
}

void TelephonyInstance::HandleMessage(const char* msg) {
  picojson::value v;
  std::string err;
  picojson::parse(v, msg, msg + strlen(msg), &err);
  if (!err.empty()) {
    std::cerr << "Error: ignoring empty message.\n";
    return;
  }

  if (!backend_) {
    SendErrorReply(v, NO_MODIFICATION_ALLOWED_ERR,
        "Telephony backend not initialized.");
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == kCmdGetServices) {
    backend_->GetServices(v);
  } else if (cmd == kCmdSetDefaultService) {
    backend_->SetDefaultService(v);
  } else if (cmd == kCmdSetServiceEnabled) {
    backend_->SetServiceEnabled(v);
  } else if (cmd == kCmdEnableNotifications) {
    backend_->EnableSignalHandlers();
  } else if (cmd == kCmdDisableNotifications) {
    backend_->DisableSignalHandlers();
  } else if (cmd == kCmdGetCalls) {
    backend_->GetCalls(v);
  } else if (cmd == kCmdDial) {
    backend_->DialCall(v);
  } else if (cmd == kCmdAccept) {
    backend_->AcceptCall(v);
  } else if (cmd == kCmdDisconnect) {
    backend_->DisconnectCall(v);
  } else if (cmd == kCmdHold) {
    backend_->HoldCall(v);
  } else if (cmd == kCmdResume) {
    backend_->ResumeCall(v);
  } else if (cmd == kCmdDeflect) {
    backend_->DeflectCall(v);
  } else if (cmd == kCmdTransfer) {
    backend_->TransferCall(v);
  } else if (cmd == kCmdSendTones) {
    backend_->SendTones(v);
  } else if (cmd == kCmdStartTone) {
    backend_->StartTone(v);
  } else if (cmd == kCmdStopTone) {
    backend_->StopTone(v);
  } else if (cmd == kCmdGetEmergencyNumbers) {
    backend_->GetEmergencyNumbers(v);
  } else if (cmd == kCmdEmergencyDial) {
    backend_->EmergencyDial(v);
  } else if (cmd == kCmdCreateConference) {
    backend_->CreateConference(v);
  } else if (cmd == kCmdGetParticipants) {
    backend_->GetConferenceParticipants(v);
  } else if (cmd == kCmdSplit) {
    backend_->SplitCall(v);
  } else {
    std::cout << "Ignoring unknown command: " << cmd;
  }
}

void TelephonyInstance::HandleSyncMessage(const char* message) {
}

void TelephonyInstance::SendErrorReply(const picojson::value& msg,
    const int error_code, const char* error_msg) {
  picojson::value::object reply;
  reply["promiseId"] = msg.get("promiseId");
  reply["cmd"] = msg.get("cmd");
  reply["isError"] = picojson::value(true);
  reply["errorCode"] = picojson::value(static_cast<double>(error_code));
  reply["errorMessage"] = error_msg ? picojson::value(error_msg) :
      msg.get("cmd");
  picojson::value v(reply);
  PostMessage(v.serialize().c_str());
}

void TelephonyInstance::SendSuccessReply(const picojson::value& msg,
    const picojson::value& value) {
  picojson::value::object reply;
  reply["promiseId"] = msg.get("promiseId");
  reply["cmd"] = msg.get("cmd");
  reply["isError"] = picojson::value(false);
  reply["returnValue"] = value;
  picojson::value v(reply);
  PostMessage(v.serialize().c_str());
}

void TelephonyInstance::SendSuccessReply(const picojson::value& msg) {
  picojson::value::object reply;
  reply["promiseId"] = msg.get("promiseId");
  reply["cmd"] = msg.get("cmd");
  reply["isError"] = picojson::value(false);
  picojson::value v(reply);
  PostMessage(v.serialize().c_str());
}

void TelephonyInstance::SendNotification(const picojson::value& msg) {
  PostMessage(msg.serialize().c_str());
}

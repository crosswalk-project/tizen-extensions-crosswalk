// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains the IPC implementation between the extension and runtime,
// and the glue code to Tizen specific backend.

#include "callhistory/callhistory.h"

const char kEntryID[] = "uid";
const char kServiceID[] = "serviceId";
const char kCallType[] = "type";
const char kCallFeatures[] = "features";
const char kRemoteParties[] = "remoteParties";
const char kForwardedFrom[] = "forwardedFrom";
const char kStartTime[] = "startTime";
const char kCallDuration[] = "duration";
const char kCallEndReason[] = "endReason";
const char kCallDirection[] = "direction";
const char kCallRecording[] = "recording";
const char kCallCost[] = "cost";
const char kCallCurrency[] = "currency";

const char kRemoteParty[] = "remoteParty";
const char kPersonID[] = "personId";
const char kExtRemoteParty[] = "remoteParties.remoteParty";
const char kExtPersonID[] = "remoteParties.personId";

const char kTizenTEL[] = "TEL";
const char kTizenXMPP[] = "XMPP";
const char kTizenSIP[] = "SIP";

const char kAnyCall[] = "CALL";
const char kVoiceCall[] = "VOICECALL";
const char kVideoCall[] = "VIDEOCALL";
const char kEmergencyCall[] = "EMERGENCYCALL";

const char kDialedCall[] = "DIALED";
const char kReceivedCall[] = "RECEIVED";
const char kUnseenMissedCall[] = "MISSEDNEW";
const char kMissedCall[] = "MISSED";
const char kRejectedCall[] = "REJECTED";
const char kBlockedCall[] = "BLOCKED";

namespace {

#define LOG_ERR(msg) std::cerr << "\n[Error] " << msg

static const unsigned int kInstanceMagic = 0xACDCBEEF;

}  // namespace

common::Extension* CreateExtension() {
  return new CallHistoryExtension;
}

// This will be generated from callhistory_api.js.
extern const char kSource_callhistory_api[];

CallHistoryExtension::CallHistoryExtension() {
  const char* entry_points[] = { nullptr };
  SetExtraJSEntryPoints(entry_points);
  SetExtensionName("tizen.callhistory");
  SetJavaScriptAPI(kSource_callhistory_api);
}

CallHistoryExtension::~CallHistoryExtension() {}

common::Instance* CallHistoryExtension::CreateInstance() {
  return new CallHistoryInstance;
}

CallHistoryInstance::CallHistoryInstance()
    : backendConnected_(false),
      listenerCount_(0),
      instanceCheck_(kInstanceMagic) {
}

CallHistoryInstance::~CallHistoryInstance() {
  UnregisterListener();
  ReleaseBackend();
}

bool CallHistoryInstance::IsValid() const {
  return (instanceCheck_ == kInstanceMagic);
}

void CallHistoryInstance::HandleSyncMessage(const char* msg) {
  LOG_ERR("Sync API not supported; message ignored '" << msg << "'\n");
}

void CallHistoryInstance::HandleMessage(const char* msg) {
  picojson::value js_cmd;
  picojson::value::object js_reply;
  std::string js_err;
  int err = UNKNOWN_ERR;

  js_reply["cmd"] = picojson::value("reply");

  picojson::parse(js_cmd, msg, msg + strlen(msg), &js_err);
  if (!js_err.empty()) {
    LOG_ERR("Error parsing JSON:" + js_err + " ['" + msg + "']\n");
    js_reply["errorCode"] = picojson::value(static_cast<double>(err));
    SendReply(js_reply);
    return;
  }

  js_reply["reply_id"] = js_cmd.get("reply_id");

  if (!CheckBackend()) {
    err = DATABASE_ERR;
    LOG_ERR("Could not connect to backend\n");
    js_reply["errorCode"] = picojson::value(static_cast<double>(err));
    SendReply(js_reply);
    return;
  }

  std::string cmd = js_cmd.get("cmd").to_str();
  if (cmd == "find")
    err = HandleFind(js_cmd, js_reply);  // returns results in js_reply
  else if (cmd == "remove")
    err = HandleRemove(js_cmd);  // only success/error
  else if (cmd == "removeBatch")
    err = HandleRemoveBatch(js_cmd);  // only success/error
  else if (cmd == "removeAll")
    err = HandleRemoveAll(js_cmd);  // only success/error
  else if (cmd == "addListener")
    err = HandleAddListener();
  else if (cmd == "removeListener")
    err = HandleRemoveListener();
  else
    err = INVALID_STATE_ERR;

  js_reply["errorCode"] = picojson::value(static_cast<double>(err));
  SendReply(js_reply);
}

void CallHistoryInstance::SendReply(picojson::value::object& js_reply) {
  picojson::value v(js_reply);
  PostMessage(v.serialize().c_str());
}

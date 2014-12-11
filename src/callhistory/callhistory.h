// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CALLHISTORY_CALLHISTORY_H_
#define CALLHISTORY_CALLHISTORY_H_

#include <time.h>
#include <string>
#include <iostream>
#include "common/extension.h"
#include "common/picojson.h"
#include "tizen/tizen.h"  // for errors and filter definitions

// For Tizen Call History API specification, see
// https://developer.tizen.org/dev-guide/2.2.1/org.tizen.web.device.apireference/tizen/callhistory.html

/*
 * Extension
 */
class CallHistoryExtension : public common::Extension {
 public:
  CallHistoryExtension();
  virtual ~CallHistoryExtension();
 private:
  // common::Extension implementation.
  virtual common::Instance* CreateInstance();
};

/*
 * Instance: interface for Tizen Mobile and Tizen IVI implementations
 */
class CallHistoryInstance : public common::Instance {
 public:
  CallHistoryInstance();
  virtual ~CallHistoryInstance();
  virtual bool IsValid() const;

 private:
  // common::Instance implementation.
  void HandleMessage(const char* msg);
  void HandleSyncMessage(const char* msg);

  void SendReply(picojson::value::object& jsreply);

  // Tizen API backend-specific call handlers
  int HandleFind(const picojson::value& msg,
                 picojson::value::object& reply);
  int HandleRemove(const picojson::value& msg);
  int HandleRemoveBatch(const picojson::value& msg);
  int HandleRemoveAll(const picojson::value& msg);
  int HandleAddListener();
  int HandleRemoveListener();

  int UnregisterListener();
  bool ReleaseBackend();
  bool CheckBackend();

  bool backendConnected_;
  unsigned int listenerCount_;
  unsigned int instanceCheck_;
};

// property names used in the JS API, for CallHistoryEntry
// used in the profile-specific implementations
// definitions in callhistory.cc
extern const char kEntryID[];
extern const char kServiceID[];
extern const char kCallType[];
extern const char kCallFeatures[];
extern const char kRemoteParties[];
extern const char kForwardedFrom[];
extern const char kStartTime[];
extern const char kCallDuration[];
extern const char kCallEndReason[];
extern const char kCallDirection[];
extern const char kCallRecording[];
extern const char kCallCost[];
extern const char kCallCurrency[];

// property names used in the JS API, for RemoteParty
extern const char kRemoteParty[];
extern const char kPersonID[];

// additional remote party specifiers accepted by filters
extern const char kExtRemoteParty[];
extern const char kExtPersonID[];

// call type values
extern const char kTizenTEL[];
extern const char kTizenXMPP[];
extern const char kTizenSIP[];

// call feature values
extern const char kAnyCall[];
extern const char kVoiceCall[];
extern const char kVideoCall[];
extern const char kEmergencyCall[];

// call direction values
extern const char kDialedCall[];
extern const char kReceivedCall[];
extern const char kUnseenMissedCall[];
extern const char kMissedCall[];
extern const char kRejectedCall[];
extern const char kBlockedCall[];

#endif  // CALLHISTORY_CALLHISTORY_H_

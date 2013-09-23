// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sstream>
#include <string>

#include "time/time_context.h"
#include "common/picojson.h"

#include "unicode/ustring.h"
#include "unicode/timezone.h"

DEFINE_XWALK_EXTENSION(TimeContext)

const char TimeContext::name[] = "tizen.time";

// This will be generated from time_api.js.
extern const char kSource_time_api[];

const char* TimeContext::GetJavaScript() {
  return kSource_time_api;
}

void TimeContext::HandleSyncMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring Sync message.\n";
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "GetLocalTimeZone")
    HandleGetLocalTimeZone(v);
  else if (cmd == "GetTimeZoneRawOffset")
    HandleGetTimeZoneRawOffset(v);
}

void TimeContext::HandleGetLocalTimeZone(const picojson::value& msg) {
  picojson::value::object o;

  UnicodeString local_timezone;
  TimeZone::createDefault()->getID(local_timezone);

  std::string localtz;
  local_timezone.toUTF8String(localtz);

  o["value"] = picojson::value(localtz);

  SetSyncReply(picojson::value(o));
}

void TimeContext::HandleGetTimeZoneRawOffset(const picojson::value& msg) {
  picojson::value::object o;

  UnicodeString* id = new UnicodeString(msg.get("value").to_str().c_str());

  std::stringstream offset;
  offset << TimeZone::createTimeZone(*id)->getRawOffset();

  o["value"] = picojson::value(offset.str());

  SetSyncReply(picojson::value(o));
}

void TimeContext::SetSyncReply(picojson::value v) {
  api_->SetSyncReply(v.serialize().c_str());
}

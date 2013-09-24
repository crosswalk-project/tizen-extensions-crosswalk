// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sstream>
#include <string>
#include <memory>
#include <cerrno>

#include "time/time_context.h"
#include "common/picojson.h"

#include "unicode/ustring.h"
#include "unicode/timezone.h"
#include "unicode/calendar.h"
#include "unicode/vtzone.h"
#include "unicode/tztrans.h"
#include "unicode/smpdtfmt.h"

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
  picojson::value::object o;
  if (cmd == "GetLocalTimeZone")
    o = HandleGetLocalTimeZone(v);
  else if (cmd == "GetTimeZoneRawOffset")
    o = HandleGetTimeZoneRawOffset(v);
  else if (cmd == "GetTimeZoneAbbreviation")
    o = HandleGetTimeZoneAbbreviation(v);
  else if (cmd == "IsDST")
    o = HandleIsDST(v);
  else if (cmd == "GetDSTTransition")
    o = HandleGetDSTTransition(v);

  if (o.empty())
    o["error"] = picojson::value(true);

  SetSyncReply(picojson::value(o));
}

const picojson::value::object TimeContext::HandleGetLocalTimeZone(
  const picojson::value& msg) {
  picojson::value::object o;

  UnicodeString local_timezone;
  TimeZone::createDefault()->getID(local_timezone);

  std::string localtz;
  local_timezone.toUTF8String(localtz);

  o["value"] = picojson::value(localtz);

  return o;
}

const picojson::value::object TimeContext::HandleGetTimeZoneRawOffset(
  const picojson::value& msg) {
  picojson::value::object o;

  std::auto_ptr<UnicodeString> id(
    new UnicodeString(msg.get("timezone").to_str().c_str()));

  std::stringstream offset;
  offset << TimeZone::createTimeZone(*id)->getRawOffset();

  o["value"] = picojson::value(offset.str());

  return o;
}

const picojson::value::object TimeContext::HandleGetTimeZoneAbbreviation(
  const picojson::value& msg) {
  picojson::value::object o;

  std::auto_ptr<UnicodeString> id(
    new UnicodeString(msg.get("timezone").to_str().c_str()));
  UDate dateInMs = strtod(msg.get("value").to_str().c_str(), NULL);

  if (errno == ERANGE)
    return o;

  UErrorCode ec = U_ZERO_ERROR;
  std::auto_ptr<Calendar> cal(
    Calendar::createInstance(TimeZone::createTimeZone(*id), ec));
  if (U_FAILURE(ec))
    return o;

  cal->setTime(dateInMs, ec);
  if (U_FAILURE(ec))
    return o;

  std::auto_ptr<DateFormat> fmt(
    new SimpleDateFormat(UnicodeString("z"), Locale::getEnglish(), ec));
  if (U_FAILURE(ec))
    return o;

  UnicodeString uAbbreviation;
  fmt->setCalendar(*cal);
  fmt->format(cal->getTime(ec), uAbbreviation);
  if (U_FAILURE(ec))
    return o;

  std::string abbreviation = "";
  uAbbreviation.toUTF8String(abbreviation);

  o["value"] = picojson::value(abbreviation);

  return o;
}

const picojson::value::object TimeContext::HandleIsDST(
  const picojson::value& msg) {
  picojson::value::object o;

  std::auto_ptr<UnicodeString> id(
    new UnicodeString(msg.get("timezone").to_str().c_str()));
  UDate dateInMs = strtod(msg.get("value").to_str().c_str(), NULL);

  if (errno == ERANGE)
    return o;

  UErrorCode ec = U_ZERO_ERROR;
  std::auto_ptr<Calendar> cal(
    Calendar::createInstance(TimeZone::createTimeZone(*id), ec));
  if (U_FAILURE(ec))
    return o;

  cal->setTime(dateInMs, ec);
  if (U_FAILURE(ec))
    return o;

  o["value"] = picojson::value(static_cast<bool>(cal->inDaylightTime(ec)));

  return o;
}

const picojson::value::object TimeContext::HandleGetDSTTransition(
  const picojson::value& msg) {
  picojson::value::object o;

  std::auto_ptr<UnicodeString> id(
    new UnicodeString(msg.get("timezone").to_str().c_str()));
  std::string trans = msg.get("trans").to_str();
  UDate dateInMs = strtod(msg.get("value").to_str().c_str(), NULL);

  if (errno == ERANGE)
    return o;

  std::auto_ptr<VTimeZone> vtimezone(VTimeZone::createVTimeZoneByID(*id));

  if (!vtimezone->useDaylightTime())
    return o;

  TimeZoneTransition tzTransition;
  if (trans.compare("NEXT_TRANSITION") &&
      vtimezone->getNextTransition(dateInMs, FALSE, tzTransition))
    o["value"] = picojson::value(tzTransition.getTime());
  else if (vtimezone->getPreviousTransition(dateInMs, FALSE, tzTransition))
    o["value"] = picojson::value(tzTransition.getTime());

  return o;
}

void TimeContext::SetSyncReply(picojson::value v) {
  api_->SetSyncReply(v.serialize().c_str());
}

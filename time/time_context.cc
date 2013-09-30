// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(TIZEN_MOBILE)
#include <vconf.h>
#endif

#include <sstream>
#include <string>
#include <memory>
#include <cerrno>

#include "time/time_context.h"
#include "common/picojson.h"

#include "unicode/timezone.h"
#include "unicode/calendar.h"
#include "unicode/vtzone.h"
#include "unicode/tztrans.h"
#include "unicode/smpdtfmt.h"
#include "unicode/dtptngen.h"

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
  else if (cmd == "GetAvailableTimeZones")
    o = HandleGetAvailableTimeZones(v);
  else if (cmd == "GetTimeZoneRawOffset")
    o = HandleGetTimeZoneRawOffset(v);
  else if (cmd == "GetTimeZoneAbbreviation")
    o = HandleGetTimeZoneAbbreviation(v);
  else if (cmd == "IsDST")
    o = HandleIsDST(v);
  else if (cmd == "GetDSTTransition")
    o = HandleGetDSTTransition(v);
  else if (cmd == "ToDateString")
    o = HandleToString(v, TimeContext::DATE_FORMAT);
  else if (cmd == "ToTimeString")
    o = HandleToString(v, TimeContext::TIME_FORMAT);
  else if (cmd == "ToString")
    o = HandleToString(v, TimeContext::DATETIME_FORMAT);

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

const picojson::value::object TimeContext::HandleGetAvailableTimeZones(
  const picojson::value& msg) {
  picojson::value::object o;

  UErrorCode ec = U_ZERO_ERROR;
  std::unique_ptr<StringEnumeration> timezones(TimeZone::createEnumeration());
  int32_t count = timezones->count(ec);

  if (U_FAILURE(ec))
    return o;

  picojson::value::array a;
  const char *timezone = NULL;
  int i = 0;
  do {
    int32_t resultLen = 0;
    timezone = timezones->next(&resultLen, ec);
    if (U_SUCCESS(ec)) {
      a.push_back(picojson::value(timezone));
      i++;
    }
  }while(timezone && i < count);

  o["value"] = picojson::value(a);

  return o;
}

const picojson::value::object TimeContext::HandleGetTimeZoneRawOffset(
  const picojson::value& msg) {
  picojson::value::object o;

  std::unique_ptr<UnicodeString> id(
    new UnicodeString(msg.get("timezone").to_str().c_str()));

  std::stringstream offset;
  offset << TimeZone::createTimeZone(*id)->getRawOffset();

  o["value"] = picojson::value(offset.str());

  return o;
}

const picojson::value::object TimeContext::HandleGetTimeZoneAbbreviation(
  const picojson::value& msg) {
  picojson::value::object o;

  std::unique_ptr<UnicodeString> id(
    new UnicodeString(msg.get("timezone").to_str().c_str()));
  UDate dateInMs = strtod(msg.get("value").to_str().c_str(), NULL);

  if (errno == ERANGE)
    return o;

  UErrorCode ec = U_ZERO_ERROR;
  std::unique_ptr<Calendar> cal(
    Calendar::createInstance(TimeZone::createTimeZone(*id), ec));
  if (U_FAILURE(ec))
    return o;

  cal->setTime(dateInMs, ec);
  if (U_FAILURE(ec))
    return o;

  std::unique_ptr<DateFormat> fmt(
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

  std::unique_ptr<UnicodeString> id(
    new UnicodeString(msg.get("timezone").to_str().c_str()));
  UDate dateInMs = strtod(msg.get("value").to_str().c_str(), NULL);

  if (errno == ERANGE)
    return o;

  UErrorCode ec = U_ZERO_ERROR;
  std::unique_ptr<Calendar> cal(
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

  std::unique_ptr<UnicodeString> id(
    new UnicodeString(msg.get("timezone").to_str().c_str()));
  std::string trans = msg.get("trans").to_str();
  UDate dateInMs = strtod(msg.get("value").to_str().c_str(), NULL);

  if (errno == ERANGE)
    return o;

  std::unique_ptr<VTimeZone> vtimezone(VTimeZone::createVTimeZoneByID(*id));

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

const picojson::value::object TimeContext::HandleToString(
  const picojson::value& msg, DateTimeFormatType format) {
  picojson::value::object o;

  std::unique_ptr<UnicodeString> id(
    new UnicodeString(msg.get("timezone").to_str().c_str()));
  bool bLocale = msg.get("locale").evaluate_as_boolean();

  UDate dateInMs = strtod(msg.get("value").to_str().c_str(), NULL);
  if (errno == ERANGE)
    return o;

  UErrorCode ec = U_ZERO_ERROR;
  std::unique_ptr<Calendar> cal(
    Calendar::createInstance(TimeZone::createTimeZone(*id), ec));
  if (U_FAILURE(ec))
    return o;

  cal->setTime(dateInMs, ec);
  if (U_FAILURE(ec))
    return o;

  std::unique_ptr<DateFormat> fmt(
    new SimpleDateFormat(getDateTimeFormat(format, bLocale),
                        (bLocale ? Locale::getDefault() : Locale::getEnglish()),
                         ec));
  if (U_FAILURE(ec))
    return o;

  UnicodeString uResult;
  fmt->setCalendar(*cal);
  fmt->format(cal->getTime(ec), uResult);
  if (U_FAILURE(ec))
    return o;

  std::string result = "";
  uResult.toUTF8String(result);

  o["value"] = picojson::value(result);

  return o;
}

UnicodeString TimeContext::getDateTimeFormat(DateTimeFormatType type,
                                             bool bLocale) {
  UErrorCode ec = U_ZERO_ERROR;
  std::unique_ptr<DateTimePatternGenerator> dateTimepattern(
     DateTimePatternGenerator::createInstance(
        (bLocale ? Locale::getDefault() : Locale::getEnglish()), ec));

  if (U_FAILURE(ec))
    return "";

  UnicodeString pattern;
  if (type == DATE_FORMAT) {
    pattern = dateTimepattern->getBestPattern(UDAT_YEAR_MONTH_WEEKDAY_DAY, ec);
  } else if (type == DATE_SHORT_FORMAT) {
    pattern = dateTimepattern->getBestPattern(UDAT_YEAR_NUM_MONTH_DAY, ec);
  } else {
    std::string skeleton;
    if (type != TIME_FORMAT)
      skeleton = UDAT_YEAR_MONTH_WEEKDAY_DAY;

#if defined(TIZEN_MOBILE)
    int value = 0;
    if (vconf_get_int(VCONFKEY_REGIONFORMAT_TIME1224, &value) == -1)
      skeleton += "hhmmss";
    else
      skeleton += "HHmmss";
#else
    skeleton += "hhmmss";
#endif

    pattern = dateTimepattern->getBestPattern(
       *(new UnicodeString(skeleton.c_str())), ec);
    if (U_FAILURE(ec))
      return "";

    if (!bLocale)
      pattern += " 'GMT'Z v'";
  }

  return pattern;
}

void TimeContext::SetSyncReply(picojson::value v) {
  api_->SetSyncReply(v.serialize().c_str());
}

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "time/time_instance.h"

#if defined(TIZEN)
#include <vconf.h>
#endif

#include <sstream>
#include <string>
#include <memory>
#include <cerrno>

#include "common/picojson.h"

#include "unicode/timezone.h"
#include "unicode/calendar.h"
#include "unicode/vtzone.h"
#include "unicode/tztrans.h"
#include "unicode/smpdtfmt.h"
#include "unicode/dtptngen.h"

namespace {

const int _hourInMilliseconds = 3600000;

}  // namespace

TimeInstance::TimeInstance() {}

TimeInstance::~TimeInstance() {}

void TimeInstance::HandleMessage(const char* message) {}

void TimeInstance::HandleSyncMessage(const char* message) {
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
  else if (cmd == "GetTimeZoneOffset")
    o = HandleGetTimeZoneOffset(v);
  else if (cmd == "GetTimeZoneAbbreviation")
    o = HandleGetTimeZoneAbbreviation(v);
  else if (cmd == "IsDST")
    o = HandleIsDST(v);
  else if (cmd == "GetDSTTransition")
    o = HandleGetDSTTransition(v);
  else if (cmd == "ToDateString")
    o = HandleToString(v, TimeInstance::DATE_FORMAT);
  else if (cmd == "ToTimeString")
    o = HandleToString(v, TimeInstance::TIME_FORMAT);
  else if (cmd == "ToString")
    o = HandleToString(v, TimeInstance::DATETIME_FORMAT);
  else if (cmd == "GetTimeFormat")
    o = HandleGetTimeFormat(v);

  if (o.empty())
    o["error"] = picojson::value(true);

  SendSyncReply(picojson::value(o).serialize().c_str());
}

const picojson::value::object TimeInstance::HandleGetLocalTimeZone(
  const picojson::value& msg) {
  picojson::value::object o;

  UnicodeString local_timezone;
  TimeZone::createDefault()->getID(local_timezone);

  std::string localtz;
  local_timezone.toUTF8String(localtz);

  o["value"] = picojson::value(localtz);

  return o;
}

const picojson::value::object TimeInstance::HandleGetAvailableTimeZones(
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

const picojson::value::object TimeInstance::HandleGetTimeZoneOffset(
  const picojson::value& msg) {
  picojson::value::object o;

  std::unique_ptr<UnicodeString> id(
    new UnicodeString(msg.get("timezone").to_str().c_str()));
  UDate dateInMs = strtod(msg.get("value").to_str().c_str(), NULL);

  if (errno == ERANGE)
    return o;

  UErrorCode ec = U_ZERO_ERROR;
  std::unique_ptr<TimeZone> timezone(TimeZone::createTimeZone(*id));
  std::unique_ptr<Calendar> cal(Calendar::createInstance(*timezone, ec));
  if (U_FAILURE(ec))
    return o;

  cal->setTime(dateInMs, ec);
  if (U_FAILURE(ec))
    return o;

  int32_t offset = timezone->getRawOffset();

  if (cal->inDaylightTime(ec))
    offset += _hourInMilliseconds;

  std::stringstream offsetStr;
  offsetStr << offset;

  o["value"] = picojson::value(offsetStr.str());

  return o;
}

const picojson::value::object TimeInstance::HandleGetTimeZoneAbbreviation(
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

const picojson::value::object TimeInstance::HandleIsDST(
  const picojson::value& msg) {
  picojson::value::object o;

  std::unique_ptr<UnicodeString> id(
    new UnicodeString(msg.get("timezone").to_str().c_str()));
  UDate dateInMs = strtod(msg.get("value").to_str().c_str(), NULL);
  dateInMs -= _hourInMilliseconds;

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

const picojson::value::object TimeInstance::HandleGetDSTTransition(
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

const picojson::value::object TimeInstance::HandleToString(
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

const picojson::value::object TimeInstance::HandleGetTimeFormat(
  const picojson::value& msg) {
  picojson::value::object o;

  UnicodeString timeFormat = getDateTimeFormat(TimeInstance::TIME_FORMAT, true);

  timeFormat = timeFormat.findAndReplace("H", "h");
  timeFormat = timeFormat.findAndReplace("a", "ap");

  timeFormat = timeFormat.findAndReplace("hh", "h");
  timeFormat = timeFormat.findAndReplace("mm", "m");
  timeFormat = timeFormat.findAndReplace("ss", "s");

  std::string result = "";
  timeFormat.toUTF8String(result);

  o["value"] = picojson::value(result);

  return o;
}


UnicodeString TimeInstance::getDateTimeFormat(DateTimeFormatType type,
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

#if defined(TIZEN)
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

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TIME_TIME_INSTANCE_H_
#define TIME_TIME_INSTANCE_H_

#include "common/extension.h"
#include "common/picojson.h"
#include "unicode/unistr.h"

class TimeInstance : public common::Instance {
 public:
  TimeInstance();
  virtual ~TimeInstance();

 private:
  // common::Instance implementation.
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* msg);

  enum DateTimeFormatType {
    TIME_FORMAT,
    DATE_FORMAT,
    DATE_SHORT_FORMAT,
    DATETIME_FORMAT
  };

  const picojson::value::object
     HandleGetLocalTimeZone(const picojson::value& msg);
  const picojson::value::object
     HandleGetAvailableTimeZones(const picojson::value& msg);
  const picojson::value::object
     HandleGetTimeZoneOffset(const picojson::value& msg);
  const picojson::value::object
     HandleGetTimeZoneAbbreviation(const picojson::value& msg);
  const picojson::value::object
     HandleIsDST(const picojson::value& msg);
  const picojson::value::object
     HandleGetDSTTransition(const picojson::value& msg);
  const picojson::value::object
     HandleToString(const picojson::value& msg, DateTimeFormatType type);
  const picojson::value::object
     HandleGetTimeFormat(const picojson::value& msg);

  UnicodeString getDateTimeFormat(DateTimeFormatType type, bool bLocale);
};

#endif  // TIME_TIME_INSTANCE_H_

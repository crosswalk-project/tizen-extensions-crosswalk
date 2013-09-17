// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

exports.getCurrentDateTime = function() {
    return new tizen.TZDate();
};

 exports.getLocalTimezone = function() {
    var minutesToUTC = (new Date()).getTimezoneOffset();
    var hoursToUTC = - (minutesToUTC / 60);
    if (hoursToUTC < 0)
      return 'GMT' + hoursToUTC;
    return 'GMT+' + hoursToUTC;
};

exports.getAvailableTimezones = function() {
    return [
      tizen.time.getLocalTimezone()
    ];
};

exports.getDateFormat = function(shortformat) {
    if (shortformat)
      return 'd/m/y';
    return 'D, M d y';
};

exports.getTimeFormat = function() {
    return 'h:m:s';
};

exports.isLeapYear = function(year) {
    if (!(year % 400))
      return true;
    if (!(year % 100))
      return false;
    if (!(year % 4))
      return true;
    return false;
};

tizen.TimeDuration = (function() {
  var TimeDurationUnit = [
    "MSECS",
    "SECS",
    "MINS",
    "HOURS",
    "DAYS"
  ];
  var TimeDuration = function(length, unit) {
    var length_ = length || 0;
    var unit_ = unit || 'MSECS';

    if (TimeDurationUnit.indexOf(unit_) == -1)
      unit_ = 'MSECS';

    var getMultiplier = function(unit) {
      if (unit == 'MSECS')
        return 1.0;
      if (unit == 'SECS')
        return 1.0 * 1000.0;
      if (unit == 'MINS')
        return 60.0 * 1000.0;
      if (unit == 'HOURS')
        return 3600.0 * 1000.0;
      return 86400.0 * 1000.0;
    }

    return {
      getMilliseconds: function() {
        return getMultiplier(unit_) * length_;
      },
      difference: function(other) {
        return new TimeDuration(this.getMilliseconds() -
                                other.getMilliseconds(), 'MSECS');
      },
      equalsTo: function(other) {
        return other.getMilliseconds() == this.getMilliseconds();
      },
      lessThan: function(other) {
        return this.getMilliseconds() < other.getMilliseconds();
      },
      greaterThan: function(other) {
        return this.getMilliseconds() > other.getMilliseconds();
      },
      toString: function() {
        return length_ + ' ' + unit_;
      }
    };
  };
  return TimeDuration;
})();

tizen.TZDate = (function() {
  var TZDate = function(year, month, day, hours, minutes, seconds, milliseconds, timezone) {
    var date_;

    if (!arguments.length)
      date_ = new Date();
    else
      date_ = new Date(year, month, day, hours, minutes, seconds, milliseconds);

    var timezone_ = timezone || tizen.time.getLocalTimezone();

    var toTimezone = function(timezone) {
        return new TZDate(date_.getFullYear(), date_.getMonth(),
              date_.getDate(), date_.getHours(), date_.getMinutes(),
              date_.getSeconds(), date_.getMilliseconds(), timezone);
    };

    return {
      getDate: function() {
        return date_.getDate();
      },
      setDate: function(date) {
        date_.setDate(date);
      },
      getDay: function() {
        return date_.getDay();
      },
      getFullYear: function() {
        return date_.getFullYear();
      },
      setFullYear: function(year) {
        date_.setFullYear(year);
      },
      getHours: function() {
        return date_.getHours();
      },
      setHours: function(hours) {
        date_.setHours(hours);
      },
      getMilliseconds: function() {
        return date_.getMilliseconds();
      },
      setMilliseconds: function(ms) {
        date_.setMilliseconds(ms);
      },
      getMonth: function() {
        return date_.getMonth();
      },
      setMonth: function(month) {
        date_.setMonth(month);
      },
      getMinutes: function() {
        return date_.getMinutes();
      },
      setMinutes: function(minutes) {
        date_.setMinutes(minutes);
      },
      getSeconds: function() {
        return date_.getSeconds();
      },
      setSeconds: function(seconds) {
        date_.setSeconds(seconds);
      },
      getUTCDate: function() {
        return date_.getUTCDate();
      },
      setUTCDate: function(date) {
        date_.setUTCDate(date);
      },
      getUTCDay: function() {
        return date_.getUTCDay();
      },
      setUTCDay: function(day) {
        date_.setUTCDay(day);
      },
      getUTCFullYear: function() {
        return date_.getUTCFullYear();
      },
      setUTCFullYear: function(year) {
        date_.setUTCFullYear(year);
      },
      getUTCHours: function() {
        return date_.getUTCHours();
      },
      setUTCHours: function(hours) {
        date_.setUTCHours(hours);
      },
      getUTCMilliseconds: function() {
        return date_.getUTCMilliseconds();
      },
      setUTCMilliseconds: function(ms) {
        date_.setUTCMilliseconds(ms);
      },
      getUTCMinutes: function() {
        return date_.getUTCMinutes();
      },
      setUTCMinutes: function(minutes) {
        date_.setUTCMinutes(minutes);
      },
      getUTCMonth: function() {
        return date_.getUTCMonth();
      },
      setUTCMonth: function(month) {
        date_.setUTCMonth(month);
      },
      getUTCSeconds: function() {
        return date_.getUTCSeconds();
      },
      setUTCSeconds: function(secs) {
        date_.setUTCSeconds(secs);
      },
      getTime: function() {
        return date_.getTime();
      },
      getTimezone: function() {
        return timezone_;
      },
      toTimezone: function(timezone) {
        return toTimezone(timezone);
      },
      toLocalTimezone: function() {
        return toTimezone(getLocalTimezone());
      },
      toUTC: function() {
        return toTimezone('GMT')
      },
      difference: function(other) {
        return new tizen.TimeDuration(this.getTime() -
                                      other.getTime(), 'MSEC');
      },
      equalsTo: function(other) {
        return this.getTime() == other.getTime();
      },
      earlierThan: function(other) {
        return this.getTime() < other.getTime();
      },
      laterThan: function(other) {
        return this.getTime() > other.getTime();
      },
      addDuration: function(duration) {
        var date = new TZDate(date_.getFullYear(), date_.getMonth(),
              date_.getDate(), date_.getHours(), date_.getMinutes(),
              date_.getSeconds(), date_.getMilliseconds(), timezone_);
        date.setMilliseconds(duration.getMilliseconds() +
              date.getMilliseconds());
        return date;
      },
      toLocaleDateString: function() {
        return date_.toLocaleDateString();
      },
      toLocaleTimeString: function() {
        return date_.toLocaleTimeString();
      },
      toLocaleString: function() {
        return date_.toLocaleString();
      },
      toDateString: function() {
        return date_.toDateString();
      },
      toTimeString: function() {
        return date_.toTimeString();
      },
      toString: function() {
        return date_.toString();
      },
      getTimezoneAbbreviation: function() {
        var minutesToUTC = (new Date()).getTimezoneOffset();
        var hoursToUTC = - (minutesToUTC / 60);
        if (hoursToUTC < 0)
          return 'GMT' + hoursToUTC;
        return 'GMT+' + hoursToUTC;
      },
      secondsFromUTC: function() {
        return date_.getTimezoneOffset() * 60;
      },
      isDST: function() {
        return false;
      },
      getPreviousDSTTransition: function() {
        return void 0;
      },
      getNextDSTTransition: function() {
        return void 0;
      }
    };
  };
  return TZDate;
})();

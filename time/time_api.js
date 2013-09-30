// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

exports.getCurrentDateTime = function() {
  return new tizen.TZDate();
};

exports.getLocalTimezone = function() {
  return _sendSyncMessage('GetLocalTimeZone').value;
};

var _availableTimezonesCache = [];

exports.getAvailableTimezones = function() {
  if (_availableTimezonesCache.length)
    return _availableTimezonesCache;
  _availableTimezonesCache = _sendSyncMessage('GetAvailableTimeZones').value;
  return _availableTimezonesCache;
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

function _throwProperTizenException(e) {
  if (e instanceof TypeError)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  else if (e instanceof RangeError)
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);
  else
    throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);
}

function _sendSyncMessage(cmd, timezone, value, trans, locale) {
  var msg = {
    'cmd': cmd,
    'timezone': timezone || '',
    'value': value || '',
    'trans': trans || '',
    'locale': locale || false
  };
  return JSON.parse(extension.internal.sendSyncMessage(JSON.stringify(msg)));
}

var TimeDurationUnit = [
  'MSECS',
  'SECS',
  'MINS',
  'HOURS',
  'DAYS'
];

tizen.TimeDuration = function(length, unit) {
  // FIXME(cmarcelo): This is a best effort to ensure that this function is
  // called as a constructor only. We may need to implement some native
  // primitive to ensure that.
  if (!this || this.constructor != tizen.TimeDuration)
    throw new TypeError;

  this.length = length || 0;
  this.unit = unit || 'MSECS';

  Object.defineProperty(this, 'length', {
    get: function() {
      return length; },
    set: function(NewValue) {
      if (NewValue != null)
        length = NewValue; }});

  Object.defineProperty(this, 'unit', {
    get: function() {
      return unit; },
    set: function(NewValue) {
      if (TimeDurationUnit.indexOf(NewValue) >= 0)
        unit = NewValue; }});

  if (TimeDurationUnit.indexOf(this.unit) == -1)
    this.unit = 'MSECS';
};

function getMultiplier(unit) {
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

function makeMillisecondsDurationObject(length) {
  var dayInMsecs = 1000 * 60 * 60 * 24;

  if ((length % dayInMsecs) == 0)
    return new tizen.TimeDuration(length / dayInMsecs, 'DAYS');

  return new tizen.TimeDuration(length, 'MSECS');
}

tizen.TimeDuration.prototype.getMilliseconds = function() {
  return getMultiplier(this.unit) * this.length;
};

tizen.TimeDuration.prototype.difference = function(other) {
  try {
    return makeMillisecondsDurationObject(this.getMilliseconds() -
                                          other.getMilliseconds());
  } catch (e) {
    _throwProperTizenException(e);
  }
};

tizen.TimeDuration.prototype.equalsTo = function(other) {
  try {
    return this.getMilliseconds() == other.getMilliseconds();
  } catch (e) {
    _throwProperTizenException(e);
  }
};

tizen.TimeDuration.prototype.lessThan = function(other) {
  try {
    return this.getMilliseconds() < other.getMilliseconds();
  } catch (e) {
    _throwProperTizenException(e);
  }
};

tizen.TimeDuration.prototype.greaterThan = function(other) {
  try {
    return this.getMilliseconds() > other.getMilliseconds();
  } catch (e) {
    _throwProperTizenException(e);
  }
};

tizen.TimeDuration.prototype.toString = function() {
  return this.length + ' ' + this.unit;
};

tizen.TZDate = function(year, month, day, hours, minutes, seconds, milliseconds, timezone) {
  // FIXME(cmarcelo): This is a best effort to ensure that this function is
  // called as a constructor only. We may need to implement some native
  // primitive to ensure that.
  if (!this || this.constructor != tizen.TZDate)
    throw new TypeError;

  this.date_;
  this.timezone_ = timezone || tizen.time.getLocalTimezone();

  var hours = hours || 0;
  var minutes = minutes || 0;
  var seconds = seconds || 0;
  var milliseconds = milliseconds || 0;

  if (!arguments.length)
    this.date_ = new Date();
  else if (arguments.length == 1 || arguments.length == 2) {
    if (arguments[0] instanceof Date)
      this.date_ = arguments[0];
    else
      this.date_ = new Date();
    if (arguments[1])
      this.timezone_ = arguments[1];
  }
  else
    this.date_ = new Date(year, month, day, hours, minutes, seconds, milliseconds);

  if (tizen.time.getAvailableTimezones().indexOf(this.timezone_) < 0)
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);
};

function getTimezoneRawOffset(timezone) {
  return _sendSyncMessage('GetTimeZoneRawOffset', timezone).value;
}

tizen.TZDate.prototype.getDate = function() {
  return this.date_.getDate();
};

tizen.TZDate.prototype.setDate = function(date) {
  this.date_.setDate(date);
};

tizen.TZDate.prototype.getDay = function() {
  return this.date_.getDay();
};

tizen.TZDate.prototype.getFullYear = function() {
  return this.date_.getFullYear();
};

tizen.TZDate.prototype.setFullYear = function(year) {
  this.date_.setFullYear(year);
};

tizen.TZDate.prototype.getHours = function() {
  return this.date_.getHours();
};

tizen.TZDate.prototype.setHours = function(hours) {
  this.date_.setHours(hours);
};

tizen.TZDate.prototype.getMilliseconds = function() {
  return this.date_.getMilliseconds();
};

tizen.TZDate.prototype.setMilliseconds = function(ms) {
  this.date_.setMilliseconds(ms);
};

tizen.TZDate.prototype.getMonth = function() {
  return this.date_.getMonth();
};

tizen.TZDate.prototype.setMonth = function(month) {
  this.date_.setMonth(month);
};

tizen.TZDate.prototype.getMinutes = function() {
  return this.date_.getMinutes();
};

tizen.TZDate.prototype.setMinutes = function(minutes) {
  this.date_.setMinutes(minutes);
};

tizen.TZDate.prototype.getSeconds = function() {
  return this.date_.getSeconds();
};

tizen.TZDate.prototype.setSeconds = function(seconds) {
  this.date_.setSeconds(seconds);
};

tizen.TZDate.prototype.getUTCDate = function() {
  return this.date_.getUTCDate();
};

tizen.TZDate.prototype.setUTCDate = function(date) {
  this.date_.setUTCDate(date);
};

tizen.TZDate.prototype.getUTCDay = function() {
  return this.date_.getUTCDay();
};

tizen.TZDate.prototype.setUTCDay = function(day) {
  this.date_.setUTCDay(day);
};

tizen.TZDate.prototype.getUTCFullYear = function() {
  return this.date_.getUTCFullYear();
};

tizen.TZDate.prototype.setUTCFullYear = function(year) {
  this.date_.setUTCFullYear(year);
};

tizen.TZDate.prototype.getUTCHours = function() {
  return this.date_.getUTCHours();
};

tizen.TZDate.prototype.setUTCHours = function(hours) {
  this.date_.setUTCHours(hours);
};

tizen.TZDate.prototype.getUTCMilliseconds = function() {
  return this.date_.getUTCMilliseconds();
};

tizen.TZDate.prototype.setUTCMilliseconds = function(ms) {
  this.date_.setUTCMilliseconds(ms);
};

tizen.TZDate.prototype.getUTCMinutes = function() {
  return this.date_.getUTCMinutes();
};

tizen.TZDate.prototype.setUTCMinutes = function(minutes) {
  this.date_.setUTCMinutes(minutes);
};

tizen.TZDate.prototype.getUTCMonth = function() {
  return this.date_.getUTCMonth();
};

tizen.TZDate.prototype.setUTCMonth = function(month) {
  this.date_.setUTCMonth(month);
};

tizen.TZDate.prototype.getUTCSeconds = function() {
  return this.date_.getUTCSeconds();
};

tizen.TZDate.prototype.setUTCSeconds = function(secs) {
  this.date_.setUTCSeconds(secs);
};

tizen.TZDate.prototype.getTime = function() {
  return this.date_.getTime();
};

tizen.TZDate.prototype.getTimezone = function() {
  return this.timezone_;
};

tizen.TZDate.prototype.toTimezone = function(timezone) {
  if (!timezone)
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);
  var d = new tizen.TZDate(new Date(this.date_.getTime()), timezone);
  return d.addDuration(new tizen.TimeDuration((getTimezoneRawOffset(timezone) * 1) +
                                              (getTimezoneRawOffset(this.timezone_) * -1)));
};

tizen.TZDate.prototype.toLocalTimezone = function() {
  return this.toTimezone(tizen.time.getLocalTimezone());
};

tizen.TZDate.prototype.toUTC = function() {
  return this.toTimezone('GMT');
};

tizen.TZDate.prototype.difference = function(other) {
  try {
    return makeMillisecondsDurationObject(this.getTime() - other.getTime());
  } catch (e) {
    _throwProperTizenException(e);
  }
};

tizen.TZDate.prototype.equalsTo = function(other) {
  try {
    return this.getTime() == other.getTime();
  } catch (e) {
    _throwProperTizenException(e);
  }
};

tizen.TZDate.prototype.earlierThan = function(other) {
  try {
    return this.getTime() < other.getTime();
  } catch (e) {
    _throwProperTizenException(e);
  }
};

tizen.TZDate.prototype.laterThan = function(other) {
  try {
    return this.getTime() > other.getTime();
  } catch (e) {
    _throwProperTizenException(e);
  }
};

tizen.TZDate.prototype.addDuration = function(duration) {
  try {
    var date = new tizen.TZDate(new Date(this.date_.getTime()), this.timezone_);
    date.setMilliseconds(duration.getMilliseconds() + date.getMilliseconds());
    return date;
  } catch (e) {
    _throwProperTizenException(e);
  }
};

tizen.TZDate.prototype.toLocaleDateString = function() {
  var result = _sendSyncMessage('ToDateString', this.timezone_,
                                this.date_.getTime(), '', true);
  if (result.error)
    return '';
  return result.value;
};

tizen.TZDate.prototype.toLocaleTimeString = function() {
  var result = _sendSyncMessage('ToTimeString', this.timezone_,
                                this.date_.getTime(), '', true);
  if (result.error)
    return '';
  return result.value;
};

tizen.TZDate.prototype.toLocaleString = function() {
  var result = _sendSyncMessage('ToString', this.timezone_,
                                this.date_.getTime(), '', true);
  if (result.error)
    return '';
  return result.value;
};

tizen.TZDate.prototype.toDateString = function() {
  var result = _sendSyncMessage('ToDateString', this.timezone_,
                                this.date_.getTime(), '', false);
  if (result.error)
    return '';
  return result.value;
};

tizen.TZDate.prototype.toTimeString = function() {
  var result = _sendSyncMessage('ToTimeString', this.timezone_,
                                this.date_.getTime(), '', false);
  if (result.error)
    return '';
  return result.value;
};

tizen.TZDate.prototype.toString = function() {
  var result = _sendSyncMessage('ToString', this.timezone_,
                                this.date_.getTime(), '', false);
  if (result.error)
    return '';
  return result.value;
};

tizen.TZDate.prototype.getTimezoneAbbreviation = function() {
  var result = _sendSyncMessage('GetTimeZoneAbbreviation', this.timezone_,
                                this.date_.getTime());

  if (result.error)
    return '';
  return result.value;
};

tizen.TZDate.prototype.secondsFromUTC = function() {
  return this.date_.getTimezoneOffset() * 60;
};

tizen.TZDate.prototype.isDST = function() {
  var result = _sendSyncMessage('IsDST', this.timezone_, this.date_.getTime());

  if (result.error)
    return false;
  return result.value;
};

tizen.TZDate.prototype.getPreviousDSTTransition = function() {
  var result = _sendSyncMessage('GetDSTTransition', this.timezone_,
                                this.date_.getTime(), 'NEXT_TRANSITION');

  if (result.error || result.value == 0)
    return undefined;
  return new tizen.TZDate(new Date(result.value), this.timezone_);
};

tizen.TZDate.prototype.getNextDSTTransition = function() {
  var result = _sendSyncMessage('GetDSTTransition', this.timezone_,
                                this.date_.getTime(), 'PREV_TRANSITION');

  if (result.error || result.value == 0)
    return undefined;
  return new tizen.TZDate(new Date(result.value), this.timezone_);
};

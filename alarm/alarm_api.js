// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function sendSyncMessage(msg) {
  return JSON.parse(extension.internal.sendSyncMessage(JSON.stringify(msg)));
}

function daysOfWeekToFlag(days) {
  var flag = 0;
  if (days instanceof Array && days.length > 0) {
    for (var i = 0, len = days.length; i < len; i++) {
      var day = days[i];
      if (day === 'SU')
        flag |= 0x01;
      else if (day === 'MO')
        flag |= 0x02;
      else if (day === 'TU')
        flag |= 0x04;
      else if (day === 'WE')
        flag |= 0x08;
      else if (day === 'TH')
        flag |= 0x10;
      else if (day === 'FR')
        flag |= 0x20;
      else if (day === 'SA')
        flag |= 0x40;
    }
  }

  return flag;
}

function flagToDaysOfWeek(flag) {
  var days = [];
  if (flag & 0x01)
    days.push('SU');
  if (flag & 0x02)
    days.push('MO');
  if (flag & 0x04)
    days.push('TU');
  if (flag & 0x08)
    days.push('WE');
  if (flag & 0x10)
    days.push('TH');
  if (flag & 0x20)
    days.push('FR');
  if (flag & 0x40)
    days.push('SA');

  return days;
}

function defineReadOnlyProperty(object, key, value, configurable) {
  configurable = configurable || false;
  Object.defineProperty(object, key, {
    configurable: configurable,
    enumerable: true,
    writable: false,
    value: value
  });
}

var OperationEnum = {
  AddAlarmAbs: 'alarm.add.absolute',
  AddAlarmRel: 'alarm.add.relative',
  GetRemainingSec: 'alarm.get.remainingsec',
  GetNextScheduledDate: 'alarm.get.nextScheduledDate',
  RemoveAlarm: 'alarm.remove',
  RemoveAllAlarm: 'alarm.removeAll',
  GetAlarm: 'alarm.getInfo',
  GetAllAlarms: 'alarm.getAllInfo'
};

defineReadOnlyProperty(exports, 'PERIOD_MINUTE', 60);
defineReadOnlyProperty(exports, 'PERIOD_HOUR', 3600);
defineReadOnlyProperty(exports, 'PERIOD_DAY', 86400);
defineReadOnlyProperty(exports, 'PERIOD_WEEK', 604800);

tizen.Alarm = function() {
  // set configurable as true so that this property can be re-defined in sub class
  defineReadOnlyProperty(this, 'id', null, true);
};
tizen.Alarm.prototype.constructor = tizen.Alarm;

tizen.AlarmAbsolute = function(date, periodOrDaysOfWeek) {
  tizen.Alarm.apply(this);

  if (date instanceof Date) {
    defineReadOnlyProperty(this, 'date', date);
  } else {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  var periodValue = null;
  var daysOfTheWeek = [];
  if (typeof periodOrDaysOfWeek === 'number' && !isNaN(periodOrDaysOfWeek)) {
    periodValue = periodOrDaysOfWeek;
  } else if (periodOrDaysOfWeek instanceof Array && periodOrDaysOfWeek.length > 0) {
    daysOfTheWeek = periodOrDaysOfWeek;
  }
  defineReadOnlyProperty(this, 'period', periodValue);
  defineReadOnlyProperty(this, 'daysOfTheWeek', daysOfTheWeek);
};

tizen.AlarmAbsolute.prototype = new tizen.Alarm();
tizen.AlarmAbsolute.prototype.constructor = tizen.AlarmAbsolute;

tizen.AlarmAbsolute.prototype.getNextScheduledDate = function() {
  if (this.id === undefined) {
    throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);
  }

  var operation = { cmd: OperationEnum.GetNextScheduledDate, alarm: this.id };
  var ret = sendSyncMessage(operation);
  if (ret.error) {
    throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);
  }

  var seconds = ret.data['nextScheduledDate'] || 0;
  return new Date(seconds * 1000);
};

tizen.AlarmRelative = function(delay, period) {
  tizen.Alarm.apply(this);

  if (typeof delay === 'number' && !isNaN(delay))
    defineReadOnlyProperty(this, 'delay', delay);
  else
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  if (typeof period === 'number' && !isNaN(period))
    defineReadOnlyProperty(this, 'period', period);
  else
    defineReadOnlyProperty(this, 'period', null);
};

tizen.AlarmRelative.prototype = new tizen.Alarm();
tizen.AlarmRelative.prototype.constructor = tizen.AlarmRelative;

tizen.AlarmRelative.prototype.getRemainingSeconds = function() {
  if (this.id === undefined)
    throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);

  var operation = { cmd: OperationEnum.GetRemainingSec, alarm: this.id };
  var ret = sendSyncMessage(operation);
  if (ret.error)
    throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);

  return ret.data['remainingSeconds'];
};

exports.add = function(alarm, applicationId, appControl) {
  var cmd = undefined;
  var alarmInfo = {};
  if (alarm instanceof tizen.AlarmAbsolute) {
    cmd = OperationEnum.AddAlarmAbs;
    // Convert milliseconds to seconds and round it.
    alarmInfo['date'] = Math.floor(alarm.date.getTime() / 1000 + 0.5);
    alarmInfo['period'] = alarm.period || 0;
    alarmInfo['daysOfTheWeek'] = daysOfWeekToFlag(alarm.daysOfTheWeek);
  } else if (alarm instanceof tizen.AlarmRelative) {
    cmd = OperationEnum.AddAlarmRel;
    alarmInfo['delay'] = alarm.delay;
    alarmInfo['period'] = alarm.period || 0;
  } else {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  var operation = {
    cmd: cmd,
    alarm: alarmInfo,
    applicationId: applicationId,
    appControl: appControl
  };
  var ret = sendSyncMessage(operation);
  if (ret.error)
    throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);

  defineReadOnlyProperty(alarm, 'id', ret.data['id']);
};

exports.remove = function(alarmId) {
  var operation = { cmd: OperationEnum.RemoveAlarm, alarm: alarmId };
  var ret = sendSyncMessage(operation);
  if (ret.error)
    throw new tizen.WebAPIException(tizen.WebAPIException.NOT_FOUND_ERR);
  console.debug('Succeed to remove alarm ' + alarmId);
};

exports.removeAll = function() {
  var operation = { cmd: OperationEnum.RemoveAllAlarm };
  var ret = sendSyncMessage(operation);
  if (ret.error)
    throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);
  console.debug('Succeed to remove all alarms');
};

function parseAlarm(data) {
  var alarm = null;
  var obj = JSON.parse(data);

  if (obj['type'] === 'absolute') {
    // Convert seconds to milliseconds and use it to create Date object.
    var date = new Date(obj['date'] * 1000);
    var daysOfTheWeek = flagToDaysOfWeek(obj['weekFlag']);
    alarm = new tizen.AlarmAbsolute(date, daysOfTheWeek.length > 0 ? daysOfTheWeek : obj['period']);
    defineReadOnlyProperty(alarm, 'id', obj['id']);
  } else if (obj['type'] === 'relative') {
    alarm = new tizen.AlarmRelative(obj['delay'], obj['period']);
    defineReadOnlyProperty(alarm, 'id', obj['id']);
  }

  return alarm;
}

exports.get = function(alarmId) {
  var operation = { cmd: OperationEnum.GetAlarm, alarm: alarmId };
  var ret = sendSyncMessage(operation);
  if (ret.error)
    throw new tizen.WebAPIException(tizen.WebAPIException.NOT_FOUND_ERR);
  else if (!ret.data)
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);

  return parseAlarm(ret.data);
};

exports.getAll = function() {
  var operation = { cmd: OperationEnum.GetAllAlarms };
  var ret = sendSyncMessage(operation);
  if (ret.error || !ret.data)
    throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);

  var alarms = [];
  for (var i = 0, len = ret.data.length; i < len; i++) {
    var alarm = parseAlarm(ret.data[i]);
    alarms.push(alarm);
  }
  return alarms;
};

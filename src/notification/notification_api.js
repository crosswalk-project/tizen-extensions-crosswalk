// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var v8tools = requireNative('v8tools');

var postMessage = function(msg) {
  extension.postMessage(JSON.stringify(msg));
};

var sendSyncMessage = function(msg) {
  return JSON.parse(extension.internal.sendSyncMessage(JSON.stringify(msg)));
};

function is_string(value) { return typeof(value) === 'string' || value instanceof String; }

var NOTIFICATION_PROPERTIES = [
  'statusType',
  'title',
  'content',
  'iconPath',
  'soundPath',
  'vibration',
  'progressType',
  'progressValue',
  'number',
  'subIconPath',
  'detailInfo',
  'ledColor',
  'ledOnPeriod',
  'ledOffPeriod',
  'backgroundImagePath',
  'thumbnails'
];

function extractNotificationProperties(notification) {
  var result = {};
  var i;
  for (i in NOTIFICATION_PROPERTIES) {
    var property = NOTIFICATION_PROPERTIES[i];
    result[property] = notification[property];
  }
  return result;
}

function NotificationCenter() {
  this.postedNotifications = [];
}

function checkDetailInfo(detailInfo) {
  if (detailInfo === undefined)
    return [];

  if ((detailInfo[0] !== undefined && !(detailInfo[0] instanceof tizen.NotificationDetailInfo)) ||
      (detailInfo[1] !== undefined && !(detailInfo[1] instanceof tizen.NotificationDetailInfo)))
    return [];
  return detailInfo.length <= 2 ? detailInfo : detailInfo.slice(0, 2);
}

NotificationCenter.prototype.onNotificationRemoved = function(id) {
  for (var i = 0; i < this.postedNotifications.length; i++) {
    if (this.postedNotifications[i].id === id) {
      this.postedNotifications.splice(i, 1);
      break;
    }
  }
};

NotificationCenter.prototype.wasPosted = function(notification) {
  var i;
  for (i = 0; i < this.postedNotifications.length; i++) {
    if (this.postedNotifications[i] === notification)
      return true;
  }
  return false;
};

NotificationCenter.prototype.postNotification = function(notification) {
  var msg = extractNotificationProperties(notification);
  msg.cmd = 'NotificationPost';
  var posted_id = sendSyncMessage(msg);

  if (posted_id == null)
    return false;

  v8tools.forceSetProperty(notification, 'id', posted_id.toString());
  v8tools.forceSetProperty(notification, 'postedTime', new Date);
  this.postedNotifications.push(notification);
  return true;
};

NotificationCenter.prototype.update = function(notification) {
  var msg = extractNotificationProperties(notification);
  msg.cmd = 'NotificationUpdate';
  msg.id = +notification.id;  // Pass ID as a number.
  return sendSyncMessage(msg);
};

NotificationCenter.prototype.getAll = function() {
  return this.postedNotifications.slice();
};

NotificationCenter.prototype.get = function(notificationId) {
  var result;
  var i;
  for (i = 0; i < this.postedNotifications.length; i++) {
    if (this.postedNotifications[i].id == notificationId) {
      result = this.postedNotifications[i];
      break;
    }
  }
  return result;
};

NotificationCenter.prototype.remove = function(notificationId) {
  // FIXME(cmarcelo): Do we need to make removals synchronous?
  this.onNotificationRemoved(notificationId);
  postMessage({
    'cmd': 'NotificationRemove',
    'id': +notificationId  // Pass ID as a number.
  });
};

NotificationCenter.prototype.removeAll = function() {
  var i;
  while (this.postedNotifications.length > 0)
    this.remove(this.postedNotifications[0].id);
};

var notificationCenter = new NotificationCenter;

extension.setMessageListener(function(msg) {
  var m = JSON.parse(msg);
  // FIXME(cmarcelo): for removals issues by JS, we are also being
  // notified, but is unnecessary.
  if (m.cmd == 'NotificationRemoved')
    notificationCenter.onNotificationRemoved(m.id);
});

function defineReadOnlyProperty(object, key, value) {
  Object.defineProperty(object, key, {
    configurable: false,
    writable: false,
    value: value
  });
}

function defineNonNullableProperty(object, key) {
  Object.defineProperty(object, key, {
    get: function(k) {
      return this[k];
    }.bind(object, key + '_'),
    set: function(k, NewValue) {
      if (NewValue != null)
        this[k] = NewValue;
    }.bind(object, key + '_'),
    enumerable: true });
}

function defineProgressTypeProperty(object, key) {
  Object.defineProperty(object, key, {
    get: function(k) {
      return this[k];
    }.bind(object, key + '_'),
    set: function(k, NewValue) {
      if (['BYTE', 'PERCENTAGE'].indexOf(NewValue) > -1)
        this[k] = NewValue;
    }.bind(object, key + '_'),
    enumerable: true });
}

function defineProgressValueProperty(object, key) {
  Object.defineProperty(object, key, {
    get: function(k) {
      return this[k];
    }.bind(object, key + '_'),
    set: function(k, NewValue) {
      if (NewValue != null) {
        if (this.progressType == 'PERCENTAGE' && (NewValue > 100 || NewValue < 0)) {
          this[k] = 100;
        } else {
          this[k] = NewValue >>> 0; // enforce unsigned long
        }
      } else {
        this[k] = null;
      }
    }.bind(object, key + '_'),
    enumerable: true });
}

tizen.NotificationDetailInfo = function(mainText, subText) {
  // FIXME(cmarcelo): This is a best effort that covers the common
  // cases. Investigate whether we can implement a primitive natively to give us
  // the information that the function was called as a constructor.
  if (!this || this.constructor != tizen.NotificationDetailInfo)
    throw new TypeError;
  if (!(is_string(mainText)))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  defineNonNullableProperty(this, 'mainText');

  this.mainText = mainText;
  this.subText = subText || null;
};

tizen.StatusNotification = function(statusType, title, dict) {
  // See comment in tizen.NotificationDetailInfo.
  if (!this || this.constructor != tizen.StatusNotification)
    throw new TypeError;

  this.title_ = title;

  defineReadOnlyProperty(this, 'id', undefined);
  defineReadOnlyProperty(this, 'postedTime', undefined);
  defineReadOnlyProperty(this, 'type', 'STATUS');

  defineProgressValueProperty(this, 'progressValue');
  defineProgressTypeProperty(this, 'progressType');

  if (['SIMPLE', 'THUMBNAIL', 'ONGOING', 'PROGRESS'].indexOf(statusType) < 0)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  defineReadOnlyProperty(this, 'statusType', statusType);

  if (!dict)
    dict = {};

  this.content = dict.content || null;
  this.iconPath = dict.iconPath || null;
  this.soundPath = dict.soundPath || null;
  this.vibration_ = Boolean(dict.vibration);
  this.appControl = dict.appControl || null;
  this.appId = dict.appId !== undefined ? dict.appId : null;
  this.progressType_ = dict.progressType || 'PERCENTAGE';
  this.progressValue = dict.progressValue !== undefined ? dict.progressValue : null;
  this.number = dict.number || null;
  this.subIconPath = dict.subIconPath || null;
  this.detailInfo = checkDetailInfo(dict.detailInfo);
  this.ledColor = dict.ledColor || null;
  this.ledOnPeriod_ = dict.ledOnPeriod || 0;
  this.ledOffPeriod_ = dict.ledOffPeriod || 0;
  this.backgroundImagePath = dict.backgroundImagePath || null;
  this.thumbnails = dict.thumbnails || [];

  defineNonNullableProperty(this, 'title');
  defineNonNullableProperty(this, 'vibration');
  defineNonNullableProperty(this, 'ledOnPeriod');
  defineNonNullableProperty(this, 'ledOffPeriod');
};

exports.post = function(notification) {
  if (!(notification instanceof tizen.StatusNotification)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  } else if (notificationCenter.wasPosted(notification)) {
    console.log('tizen.notification.post(): notification ' + notification.id + ' already posted.');
    return;
  }
  if (!notificationCenter.postNotification(notification))
    throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);
};

exports.remove = function(notificationId) {
  if (!notificationCenter.get(notificationId))
    throw new tizen.WebAPIException(tizen.WebAPIException.NOT_FOUND_ERR);
  notificationCenter.remove(notificationId);
};

exports.getAll = function() {
  return notificationCenter.getAll();
};

exports.get = function(notificationId) {
  var notification = notificationCenter.get(notificationId);
  if (!notification)
    throw new tizen.WebAPIException(tizen.WebAPIException.NOT_FOUND_ERR);
  return notification;
};

exports.removeAll = function() {
  notificationCenter.removeAll();
};

exports.update = function(notification) {
  if (!(notification instanceof tizen.StatusNotification))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  else if (!notificationCenter.wasPosted(notification))
    throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);
  notificationCenter.update(notification);
};

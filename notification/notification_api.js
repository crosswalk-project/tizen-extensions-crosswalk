// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var v8tools = requireNative('v8tools');

function NotificationCenter() {
  this.postedNotifications = [];
  this.statusNotificationNextId = 0;
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
    if (this.postedNotifications[i].original === notification)
      return true;
  }
  return false;
};

NotificationCenter.prototype.postNotification = function(notification) {
  var id = (this.statusNotificationNextId++).toString();
  v8tools.forceSetProperty(notification, 'id', id);

  postMessage({
    'cmd': 'NotificationPost',
    'id': notification.id,
    'title': notification.title,
    'content': notification.content
  });
  v8tools.forceSetProperty(notification, 'postedTime', new Date);

  var posted = copyStatusNotification(notification);
  posted.original = notification;
  this.postedNotifications.push(posted);
};

NotificationCenter.prototype.getAll = function() {
  var result = [];
  var i;
  for (i = 0; i < this.postedNotifications.length; i++)
    result[i] = this.postedNotifications[i].original;
  return result;
};

NotificationCenter.prototype.get = function(notificationId) {
  var result;
  var i;
  for (i = 0; i < this.postedNotifications.length; i++) {
    if (this.postedNotifications[i].id == notificationId) {
      result = this.postedNotifications[i].original;
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
    'id': notificationId
  });
};

NotificationCenter.prototype.removeAll = function() {
  var i;
  while (this.postedNotifications.length > 0)
    this.remove(this.postedNotifications[0].id);
};

var notificationCenter = new NotificationCenter;

var postMessage = function(msg) {
  extension.postMessage(JSON.stringify(msg));
};

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

tizen.NotificationDetailInfo = function(mainText, subText) {
  // FIXME(cmarcelo): This is a best effort that covers the common
  // cases. Investigate whether we can implement a primitive natively to give us
  // the information that the function was called as a constructor.
  if (!this || this.constructor != tizen.NotificationDetailInfo)
    throw new TypeError;
  this.mainText = mainText;
  this.subText = subText || null;
};

tizen.StatusNotification = function(statusType, title, dict) {
  // See comment in tizen.NotificationDetailInfo.
  if (!this || this.constructor != tizen.StatusNotification)
    throw new TypeError;

  this.title = title;

  defineReadOnlyProperty(this, 'id', undefined);
  defineReadOnlyProperty(this, 'postedTime', null);
  defineReadOnlyProperty(this, 'type', 'STATUS');

  if (['SIMPLE', 'THUMBNAIL', 'ONGOING', 'PROGRESS'].indexOf(statusType) < -1)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  defineReadOnlyProperty(this, 'statusType', statusType);

  if (!dict)
    dict = {};

  this.content = dict.content || null;
  this.iconPath = dict.iconPath || null;
  this.soundPath = dict.soundPath || null;
  this.vibration = Boolean(dict.vibration);
  this.appControl = dict.appControl || null;
  this.appId = dict.appId !== undefined ? dict.appId : null;
  this.progressType = dict.progressType || 'PERCENTAGE';
  this.progressValue = dict.progressValue !== undefined ? dict.progressValue : null;
  this.number = dict.number || null;
  this.subIconPath = dict.subIconPath || null;
  // FIXME(cmarcelo): enforce maximum of 2 elements in the array.
  this.detailInfo = dict.detailInfo || [];
  this.ledColor = dict.ledColor;
  this.ledOnPeriod = dict.ledOnPeriod || 0;
  this.ledOffPeriod = dict.ledOffPeriod || 0;
  this.backgroundImagePath = dict.backgroundImagePath || null;
  this.thumbnails = dict.thumbnails || [];
};

var copyStatusNotification = function(notification) {
  var copy = new tizen.StatusNotification(notification.statusType, notification.title, {
    content: notification.content,
    iconPath: notification.iconPath,
    soundPath: notification.soundPath,
    vibration: notification.vibration,
    appControl: notification.appControl,
    appId: notification.appId,
    progressType: notification.progressType,
    progressValue: notification.progressValue,
    number: notification.number,
    subIconPath: notification.subIconPath,
    ledColor: notification.ledColor,
    ledOnPeriod: notification.ledOnPeriod,
    ledOffPeriod: notification.ledOffPeriod,
    backgroundImagePath: notification.backgroundImagePath,
    thumbnails: notification.thumbnails
  });

  v8tools.forceSetProperty(copy, 'id', notification.id);
  v8tools.forceSetProperty(copy, 'postedTime', notification.postedTime);
  copy.detailInfo = [];
  if (notification.detailInfo) {
    var i;
    for (i = 0; i < notification.detailInfo.length; i++) {
      var info = notification.detailInfo[i];
      copy.detailInfo[i] = {
        mainText: info.mainText,
        subText: info.subText
      };
    }
  }
  return copy;
};

exports.post = function(notification) {
  if (!(notification instanceof tizen.StatusNotification)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  } else if (notificationCenter.wasPosted(notification)) {
    console.log('tizen.notification.post(): notification ' + notification.id + ' already posted.');
    return;
  }
  notificationCenter.postNotification(notification);
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

  console.log('tizen.notification.update() not implemented');
};

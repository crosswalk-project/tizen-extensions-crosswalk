// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var v8tools = requireNative("v8tools");

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
}

NotificationCenter.prototype.wasPosted = function(notification) {
  var i;
  for (i = 0; i < this.postedNotifications.length; i++) {
    if (this.postedNotifications[i].original === notification)
      return true;
  }
  return false;
}

NotificationCenter.prototype.postNotification = function(notification) {
  var id = (this.statusNotificationNextId++).toString();
  v8tools.forceSetProperty(notification, "id", id);

  postMessage({
    "cmd": "NotificationPost",
    "id": notification.id,
    "title": notification.title,
    "content": notification.content,
  });
  v8tools.forceSetProperty(notification, "postedTime", new Date);

  var posted = copyStatusNotification(notification);
  posted.original = notification;
  this.postedNotifications.push(posted);
}

NotificationCenter.prototype.getAll = function() {
  var result = [];
  var i;
  for (i = 0; i < this.postedNotifications.length; i++)
    result[i] = this.postedNotifications[i].original;
  return result;
}

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
}

NotificationCenter.prototype.remove = function(notificationId) {
  postMessage({
    "cmd": "NotificationRemove",
    "id": notificationId,
  });
}

NotificationCenter.prototype.removeAll = function() {
  var i;
  for (i = 0; i < this.postedNotifications.length; i++)
    this.remove(this.postedNotifications[i].id);
}

var notificationCenter = new NotificationCenter;

var postMessage = function(msg) {
  extension.postMessage(JSON.stringify(msg));
};

extension.setMessageListener(function(msg) {
  var m = JSON.parse(msg);
  if (m.cmd == "NotificationRemoved")
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
  this.mainText = mainText;
  this.subText = subText || null;
}

tizen.StatusNotification = function(statusType, title, dict) {
  this.title = title;

  defineReadOnlyProperty(this, "id", null);
  defineReadOnlyProperty(this, "postedTime", null);
  defineReadOnlyProperty(this, "type", "STATUS");

  if (dict) {
    this.content = dict.content;
    this.iconPath = dict.iconPath;
    this.soundPath = dict.soundPath;
    this.vibration = dict.vibration;
    this.appControl = dict.appControl;
    this.appId = dict.appId;
    this.progressType = dict.progressType;
    this.progressValue = dict.progressValue;
    this.number = dict.number;
    this.subIconPath = dict.subIconPath;
    this.detailInfo = dict.detailInfo;
    this.ledColor = dict.ledColor;
    this.ledOnPeriod = dict.ledOnPeriod;
    this.backgroundImagePath = dict.backgroundImagePath;
    this.thumbnails = dict.thumbnails;
  }
}

var copyStatusNotification = function(notification) {
  var copy = new tizen.StatusNotification(notification.type, notification.title, {
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
    backgroundImagePath: notification.backgroundImagePath,
    thumbnails: notification.thumbnails
  });

  v8tools.forceSetProperty(copy, "id", notification.id);
  v8tools.forceSetProperty(copy, "postedTime", notification.postedTime);
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
}

exports.post = function(notification) {
  if (!(notification instanceof tizen.StatusNotification)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  } else if (notificationCenter.wasPosted(notification)) {
    console.log("tizen.notification.post(): notification " + notification.id + " already posted.");
    return;
  }
  notificationCenter.postNotification(notification);
}

exports.remove = function(notificationId) {
  notificationCenter.remove(notificationId);
}

exports.getAll = function() {
  return notificationCenter.getAll();
}

exports.get = function(notificationId) {
  return notificationCenter.get(notificationId);
}

exports.removeAll = function() {
  notificationCenter.removeAll();
}

exports.update = function(notification) {
  if (!(notification instanceof tizen.StatusNotification))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  else if (!notificationCenter.wasPosted(notification))
    throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);

  console.log("tizen.notification.update() not implemented");
}

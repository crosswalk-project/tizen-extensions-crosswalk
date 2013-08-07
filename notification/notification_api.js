// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// FIXME(cmarcelo): Needed?
var postedNotifications = [];
var statusNotificationNextId = 0;

var postMessage = function(msg) {
  extension.postMessage(JSON.stringify(msg));
};

extension.setMessageListener(function(msg) {
  var m = JSON.parse(msg);
  if (m.cmd == "NotificationRemoved") {
    for (var i = 0; i < postedNotifications.length; i++) {
      if (postedNotifications[i].id === m.id) {
        postedNotifications.splice(i, 1);
        break;
      }
    }
  }
});

function defineReadOnlyProperty(object, key, value) {
  Object.defineProperty(object, key, {
    configurable: false,
    writable: false,
    value: value
  });
}

// FIXME(cmarcelo): Some properties are readonly but expect a null value before
// are set, and also changes when reused. In the future StatusNotification
// should point to an internal data and we should set a getter. Until then, this
// at least make the property non-writable.
function defineConfigurableReadOnlyProperty(object, key, value) {
  Object.defineProperty(object, key, {
    configurable: true,
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

  defineConfigurableReadOnlyProperty(this, "id", null);
  defineConfigurableReadOnlyProperty(this, "postedTime", null);
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

  defineConfigurableReadOnlyProperty(copy, "id", notification.id);
  defineConfigurableReadOnlyProperty(copy, "postedTime", notification.postedTime);
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

function isAlreadyPosted(notification) {
  var i;
  for (i = 0; i < postedNotifications.length; i++) {
    if (postedNotifications[i].original === notification)
      return true;
  }
  return false;
}

exports.post = function(notification) {
  if (!(notification instanceof tizen.StatusNotification)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  } else if (isAlreadyPosted(notification)) {
    console.log("tizen.notification.post(): notification " + notification.id + " already posted.");
    return;
  }

  var id = (statusNotificationNextId++).toString();
  defineConfigurableReadOnlyProperty(notification, "id", id);

  postMessage({
    "cmd": "NotificationPost",
    "id": notification.id,
    "title": notification.title,
    "content": notification.content,
  });
  defineConfigurableReadOnlyProperty(notification, "postedTime", new Date);

  var posted = copyStatusNotification(notification);
  posted.original = notification;
  postedNotifications.push(posted);
}

exports.remove = function(notificationId) {
  postMessage({
    "cmd": "NotificationRemove",
    "id": notificationId,
  });
}

exports.getAll = function() {
  var result = [];
  var i;
  for (i = 0; i < postedNotifications.length; i++)
    result[i] = postedNotifications[i].original;
  return result;
}

exports.get = function(notificationId) {
  var result;
  var i;
  for (i = 0; i < postedNotifications.length; i++) {
    if (postedNotifications[i].id == notificationId) {
      result = postedNotifications[i].original;
      break;
    }
  }
  return result;
}

exports.removeAll = function() {
  var i;
  for (i = 0; i < postedNotifications.length; i++)
    exports.remove(postedNotifications[i].id);
}

exports.update = function(notification) {
  if (!(notification instanceof tizen.StatusNotification))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  else if (!isAlreadyPosted(notification))
    throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);

  console.log("Not implemented");
}

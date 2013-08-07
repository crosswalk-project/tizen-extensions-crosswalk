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

tizen.NotificationDetailInfo = function(mainText, subText) {
  this.mainText = mainText;
  this.subText = subText || null;
}

tizen.StatusNotification = function(statusType, title, dict) {
  this.title = title;
  this.id = (statusNotificationNextId++).toString();
  this.type = "STATUS";
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
  statusNotificationNextId--;  // Roll next id back since we are copying.
  copy.id = notification.id;
  copy.postedTime = notification.postedTime;
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
    console.log("tizen.notification.post(): argument of invalid type " + typeof(notification));
    return;
  } else if (isAlreadyPosted(notification)) {
    console.log("tizen.notification.post(): notification " + notification.id + " already posted.");
    return;
  }
  postMessage({
    "cmd": "NotificationPost",
    "id": notification.id,
    "title": notification.title,
    "content": notification.content,
  });
  notification.postedTime = new Date;

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
  if (!(notification instanceof tizen.StatusNotification)) {
    console.log("tizen.notification.update(): argument of invalid type " + typeof(notification));
    return;
  }

  var notificationIndex = postedNotifications.indexOf(notification);
  if (notificationIndex == -1) {
    console.log("tizen.notification.update(): notification " + notification.id + " not yet posted.");
    // FIXME(jeez): needed or should we post it then?
    return;
  }

  postedNotifications[notificationIndex] = notification;

  postMessage({
    "cmd": "NotificationPost",
    "id": notification.id,
    "title": notification.title,
    "content": notification.content,
  });
}

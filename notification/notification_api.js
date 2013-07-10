// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var tizen = tizen || {};

(function() {
  // FIXME(cmarcelo): Needed?
  var postedNotifications = [];
  var statusNotificationNextId = 0;

  var postMessage = function(msg) {
    xwalk.postMessage("tizen.notifications", JSON.stringify(msg));
  };

  xwalk.setMessageListener('tizen.notifications', function(msg) {
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

  tizen.StatusNotification = function(statusType, title, dict) {
    this.title = title;
    this.id = (statusNotificationNextId++).toString();
    this.type = "STATUS";
    if (dict) {
      this.content = dict.content;
    }
  }

  var copyStatusNotification = function(notification) {
    var copy = new tizen.StatusNotification(notification.type, notification.title, { content: notification.content });
    statusNotificationNextId--;  // Roll next id back since we are copying.
    copy.id = notification.id;
    return copy;
  }

  tizen.notification = {};
  tizen.notification.post = function(notification) {
    if (!(notification instanceof tizen.StatusNotification)) {
      console.log("tizen.notification.post(): argument of invalid type " + typeof(notification));
      return;
    } else if (postedNotifications.indexOf(notification) != -1) {
      console.log("tizen.notification.post(): notification " + notification.id + " already posted.");
      return;
    }
    postMessage({
      "cmd": "NotificationPost",
      "id": notification.id,
      "title": notification.title,
      "content": notification.content,
    });
    postedNotifications.push(notification);
  }

  tizen.notification.remove = function(notificationId) {
    postMessage({
      "cmd": "NotificationRemove",
      "id": notificationId,
    });
  }

  tizen.notification.getAll = function() {
    var result = [];
    var i;
    for (i = 0; i < postedNotifications.length; i++)
      result[i] = copyStatusNotification(postedNotifications[i]);
    return result;
  }

  tizen.notification.get = function(notificationId) {
    var result;
    var i;
    for (i = 0; i < postedNotifications.length; i++) {
      if (postedNotifications[i].id == notificationId) {
        result = copyStatusNotification(postedNotifications[i]);
        break;
      }
    }
    return result;
  }

  tizen.notification.removeAll = function() {
    var i;
    for (i = 0; i < postedNotifications.length; i++)
      tizen.notification.remove(postedNotifications[i].id);
  }

  tizen.notification.update = function(notification) {
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
})();

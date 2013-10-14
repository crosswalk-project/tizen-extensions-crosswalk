// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var _callbacks = {};
var _next_reply_id = 0;
var _listeners = {};
var _next_listener_id = 0;

var postMessage = function(msg, callback) {
  var reply_id = _next_reply_id;
  _next_reply_id += 1;
  _callbacks[reply_id] = callback;
  msg._reply_id = reply_id.toString();
  extension.postMessage(JSON.stringify(msg));
};

exports.getCPUInfo = function(callback) {
  var msg = {
    'cmd': 'getCPUInfo'
  };
  postMessage(msg, callback);
};

exports.getDisplayInfo = function(callback) {
  var msg = {
    'cmd': 'getDisplayInfo'
  };
  postMessage(msg, callback);
};

exports.getMemoryInfo = function(callback) {
  var msg = {
    'cmd': 'getMemoryInfo'
  };
  postMessage(msg, callback);
};

exports.getStorageInfo = function(callback) {
  var msg = {
    'cmd': 'getStorageInfo'
  };
  postMessage(msg, callback);
};

function _addConstProperty(obj, propertyKey, propertyValue) {
  Object.defineProperty(obj, propertyKey, {
    configurable: false,
    writable: false,
    value: propertyValue
  });
}

function _createConstClone(obj) {
  var const_obj = {};
  for (var key in obj) {
    if (Array.isArray(obj[key])) {
      var obj_array = obj[key];
      var const_obj_array = [];
      for (var i = 0; i < obj_array.length; ++i) {
        var const_sub_obj = {};
        for (var sub_key in obj_array[i]) {
          _addConstProperty(const_sub_obj, sub_key, obj_array[i][sub_key]);
        }
        const_obj_array.push(const_sub_obj);
      }
      _addConstProperty(const_obj, key, const_obj_array);
    } else {
      _addConstProperty(const_obj, key, obj[key]);
    }
  }
  return const_obj;
}

extension.setMessageListener(function(json) {
  var msg = JSON.parse(json);

  if (msg.cmd == 'attachStorage' ||
      msg.cmd == 'detachStorage' ||
      msg.cmd == 'connectDisplay' ||
      msg.cmd == 'disconnectDisplay') {
      for (var id in _listeners) {
        if (_listeners[id]['eventName'] === msg.eventName) {
          _listeners[id]['callback'](_createConstClone(msg));
        }
      }
    return;
  }

  var reply_id = msg._reply_id;
  var callback = _callbacks[reply_id];
  if (typeof callback === 'function') {
    delete msg.reply_id;
    delete _callbacks[reply_id];
    callback(_createConstClone(msg));
  } else {
    console.log('Invalid reply_id: ' + reply_id);
  }
});

var _hasListener = function(eventName) {
  var count = 0;

  for (var i in _listeners) {
    if (_listeners[i]['eventName'] === eventName) {
      count += 1;
    }
  }

  return (0 !== count);
};

exports.addEventListener = function(eventName, callback) {
  if (typeof eventName !== 'string') {
    console.log("Invalid parameters (*, -)!");
    return -1;
  }

  if (typeof callback !== 'function') {
    console.log("Invalid parameters (-, *)!");
    return -1;
  }

  if (!_hasListener(eventName)) {
    var msg = {
      'cmd': 'addEventListener',
      'eventName': eventName
    };
    extension.postMessage(JSON.stringify(msg));
  }

  var listener = {
    'eventName': eventName,
    'callback': callback
  };

  var listener_id = _next_listener_id;
  _next_listener_id += 1;
  _listeners[listener_id] = listener;

  return listener_id;
};

var _sendSyncMessage = function(msg) {
  return extension.internal.sendSyncMessage(JSON.stringify(msg));
};

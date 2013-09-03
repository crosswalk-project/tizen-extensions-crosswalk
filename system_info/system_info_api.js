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

var _checkThreshold = function(value, highThreshold, lowThreshold) {
  if ((highThreshold && (highThreshold >= 0) && (value >= highThreshold)) ||
      (lowThreshold && (lowThreshold >= 0) && (value <= lowThreshold)))
    return true;

  return false;
}

extension.setMessageListener(function(json) {
  var msg = JSON.parse(json);

  // For listeners 
  if (msg.cmd == "SystemInfoPropertyValueChanged") {
    if (msg.prop && (0 !== msg.prop.length)) {
      for (var id in _listeners) {
        if (_listeners[id]["prop"] === msg.prop) {
          var option = _listeners[id]["option"];
          if (option) {
            var currentTime = (new Date()).valueOf();
            var timeout = parseFloat(option["timeout"]);
            var highThreshold = parseFloat(option["highThreshold"]);
            var lowThreshold = parseFloat(option["lowThreshold"]);
            var timeStamp = parseFloat(_listeners[id]["timestamp"]);
            if (timeout && (currentTime - timeStamp) > timeout) {
              delete _listeners[id];
              if (!_hasListener(msg.prop)) {
                var message = {
                  'cmd': 'stopListening',
                  'prop': msg.prop,
                };
                extension.postMessage(JSON.stringify(message));
                return;
              }
              continue;
            }
            switch(msg.prop) {
              case "BATTERY":
                if (_checkThreshold(msg.data.level, highThreshold, lowThreshold))
                  _listeners[id]["callback"](msg.data);
                break;
              case "CPU":
                if (_checkThreshold(msg.data.load, highThreshold, lowThreshold))
                  _listeners[id]["callback"](msg.data);
                break;
              case "DISPLAY":
                if (_checkThreshold(msg.data.brightness, highThreshold, lowThreshold))
                  _listeners[id]["callback"](msg.data);
                break;
              case "STORAGE":
              case "DEVICE_ORIENTATION":
              case "BUILD":
              case "LOCALE":
              case "NETWORK":
              case "WIFI_NETWORK":
              case "CELLULAR_NETWORK":
              case "SIM":
              case "PERIPHERAL":
                _listeners[id]["callback"](msg.data);
            }
            _listeners[id]["timestamp"] = currentTime;
            continue;
          }
          _listeners[id]["callback"](msg.data);
        }
      }
    }
    return;
  }

  // For getPropertyValue
  var reply_id = msg._reply_id;
  var callback = _callbacks[reply_id];
  if (callback) {
    delete msg._reply_id;
    delete _callbacks[reply_id];
    callback(msg);
  } else {
    console.log('Invalid reply_id received from tizen.systeminfo extension: ' + reply_id);
  }
});

exports.getCapabilities = function() {
  var capbilities = JSON.parse(_sendSyncMessage({
    "cmd": "getCapabilities",
  }));
  if (capbilities['error']) {
    throw new tizen.WebAPIException(tizen.WebAPIException.NOT_SUPPORTED_ERR);
  } else {
    delete capbilities['error'];
    return capbilities;
  }
}

var _sendSyncMessage = function(msg) {
  return extension.internal.sendSyncMessage(JSON.stringify(msg));
};

var _getPropertyValue = function(prop, callback) {
  var msg = {
    'cmd': 'getPropertyValue',
    'prop': prop,
  };
  postMessage(msg, function(r) {
    callback(r.error, r.data);
  });
};

exports.getPropertyValue = function(prop, successCallback, errorCallback) {
  if (!successCallback) {
    return;
  }
  _getPropertyValue(prop, function(error, data) {
    if (!error) {
      successCallback(data);
    } else if (errorCallback) {
      errorCallback(error);
    }
  });
};

var _hasListener = function(prop) {
  var count = 0;

  for (var i in _listeners) {
    if (_listeners[i]['prop'] === prop) {
      count += 1;
    }
  }

  return (0 !== count);
}

exports.addPropertyValueChangeListener = function(prop, successCallback, option) {
  if (typeof prop !== 'string')
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  if (typeof successCallback !== 'function')
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  if (option && (typeof option !== 'object'))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  if (!_hasListener(prop)) {
    var msg = {
      'cmd': 'startListening',
      'prop': prop,
    };
    extension.postMessage(JSON.stringify(msg));
  }

  var timeStamp = (new Date()).valueOf();
  var listener = {
    "prop": prop,
    "callback": successCallback,
    "option": option,
    "timestamp": timeStamp,
  };

  var listener_id = _next_listener_id;
  _next_listener_id += 1;
  _listeners[listener_id] = listener;

  return listener_id;
}

exports.removePropertyValueChangeListener = function(listenerId) {
  var prop = _listeners[listenerId]["prop"];

  delete _listeners[listenerId];
  if (!_hasListener(prop)) {
    var msg = {
      'cmd': 'stopListening',
      'prop': prop,
    };
    extension.postMessage(JSON.stringify(msg));
  }
}

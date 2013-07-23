// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var _callbacks = {};
var _next_reply_id = 0;

var postMessage = function(msg, callback) {
  var reply_id = _next_reply_id;
  _next_reply_id += 1;
  _callbacks[reply_id] = callback;
  msg._reply_id = reply_id.toString();
  extension.postMessage(JSON.stringify(msg));
};

extension.setMessageListener(function(json) {
  var msg = JSON.parse(json);
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
  var msg = {
    'cmd': 'getCapabilities',
  };
  // FIXME(halton): sync api is not supported on PostMessage
  console.log('NOT_IMPLEMENTED(tizen.systeminfo.getCapabilities)');
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

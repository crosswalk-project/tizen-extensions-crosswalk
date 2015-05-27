// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
var systemSettingTypes = {
  'HOME_SCREEN': 0,
  'LOCK_SCREEN': 1,
  'INCOMING_CALL': 2,
  'NOTIFICATION_EMAIL': 3,
  'LOCALE' : 4
};
var _callbacks = {};
var _next_reply_id = 0;
extension.setMessageListener(function(message) {
  var m = JSON.parse(message);
  var _reply_id = m._reply_id;
  var _error = m._error;
  var handler = _callbacks[_reply_id];
  if (handler) {
    delete m._reply_id;
    delete m._error;
    delete _callbacks[_reply_id];
    if (!_error) {
      handler[0](m._file);
    } else if (handler[1]) {
      handler[1](new tizen.WebAPIError(tizen.WebAPIException.UNKNOWN_ERR));
    }
    delete m._file;
  } else {
    console.log('Invalid reply_id received from xwalk.systemsetting extension:' +
                _reply_id);
  }
});

exports.setProperty = function(type, proposedPath, successCallback, errorCallback) {
  if (typeof type !== 'string' || !(type in systemSettingTypes))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  if (typeof proposedPath !== 'string')
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  if (typeof (successCallback) !== 'function')
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  if (arguments.length == 4 && typeof (errorCallback) !== 'function')
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var id = (_next_reply_id++).toString();
  _callbacks[id] = [successCallback, errorCallback];
  extension.postMessage(JSON.stringify({
    'cmd': 'SetProperty',
    '_type': systemSettingTypes[type],
    '_file': proposedPath,
    '_reply_id': id
  }));
};

exports.getProperty = function(type, successCallback, errorCallback) {
  if (typeof (type) !== 'string' || !(type in systemSettingTypes))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  if (typeof (successCallback) !== 'function')
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  if (arguments.length == 3 && typeof (errorCallback) !== 'function')
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var id = (_next_reply_id++).toString();
  _callbacks[id] = [successCallback, errorCallback];
  extension.postMessage(JSON.stringify({
    'cmd': 'GetProperty',
    '_type': systemSettingTypes[type],
    '_reply_id': id
  }));
};

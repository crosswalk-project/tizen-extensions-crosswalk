// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var NetworkType = {
  'CELLULAR' : 0,
  'UNKNOWN' : 1
};

var _callbacks = {};
var _next_reply_id = 0;

function _messageListener(msg) {
  var m = JSON.parse(msg);
  var handler = _callbacks[m.reply_id];
  var no_error = tizen.WebAPIError.NO_ERROR;

  if (!(m.cmd == 'requestRouteToHost' && m.disconnected == false && m.error == no_error))
    delete _callbacks[m.reply_id];

  if (m.error != no_error && handler[1]) {
    handler[1](new tizen.WebAPIError(m.error));
    return;
  }

  if (m.disconnected) {
    handler[0].ondisconnected();
    return;
  }

  if (m.cmd == 'releaseRouteToHost')
    handler[0]();
  else
    handler[0].onsuccess();
}

function _typeMismatchError() {
  throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
}

function _routeToHost(cmd, networkType, domainName, successCallback, errorCallback) {
  if (!networkType in NetworkType)
    _typeMismatchError();

  if (typeof domainName !== 'string')
    _typeMismatchError();

  if (errorCallback && typeof errorCallback !== 'function')
    _typeMismatchError();

  if (cmd == 'requestRouteToHost') {
    if (typeof successCallback !== 'object')
      _typeMismatchError();

    if (!successCallback.hasOwnProperty('onsuccess'))
      _typeMismatchError();

    if (typeof successCallback.onsuccess !== 'function')
      _typeMismatchError();

    if (!successCallback.hasOwnProperty('ondisconnected'))
      _typeMismatchError();

    if (typeof successCallback.ondisconnected !== 'function')
      _typeMismatchError();
  } else {
    if (typeof successCallback !== 'function')
      _typeMismatchError();
  }

  var id = (_next_reply_id++).toString();
  _callbacks[id] = [successCallback, errorCallback];

  extension.postMessage(JSON.stringify({
    'cmd': cmd,
    'network_type': NetworkType[networkType],
    'domain_name': domainName,
    'reply_id': id
  }));
}

extension.setMessageListener(_messageListener);

exports.requestRouteToHost = function(networkType, domainName, successCallback, errorCallback) {
  _routeToHost('requestRouteToHost', networkType, domainName, successCallback, errorCallback);
};

exports.releaseRouteToHost = function(networkType, domainName, successCallback, errorCallback) {
  _routeToHost('releaseRouteToHost', networkType, domainName, successCallback, errorCallback);
};

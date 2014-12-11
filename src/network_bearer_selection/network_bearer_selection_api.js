// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var NetworkType = {
  'CELLULAR' : 0,
  'UNKNOWN' : 1
};

var _callbacks = {};
var _next_reply_id = 0;

function _messageListener(data) {
  var msg = JSON.parse(data);
  var callbacks = _callbacks[msg.reply_id];

  if (msg.cmd == 'requestRouteToHost')
    _handleRequestRouteToHost(callbacks[0], callbacks[1], msg);
  else
    _handleReleaseRouteToHost(callbacks[0], callbacks[1], msg);
}

function _handleRequestRouteToHost(networkSuccessCallback, errorCallback, msg) {
  if (msg.error != tizen.WebAPIError.NO_ERROR) {
    if (errorCallback)
      errorCallback(new tizen.WebAPIError(msg.error));

    delete _callbacks[msg.reply_id];
    return;
  }

  if (msg.disconnected) {
    if (networkSuccessCallback.ondisconnected)
      networkSuccessCallback.ondisconnected();

    delete _callbacks[msg.reply_id];
    return;
  }

  if (networkSuccessCallback.onsuccess)
    networkSuccessCallback.onsuccess();
}

function _handleReleaseRouteToHost(successCallback, errorCallback, msg) {
  delete _callbacks[msg.reply_id];

  if (msg.error != tizen.WebAPIError.NO_ERROR) {
    if (errorCallback)
      errorCallback(new tizen.WebAPIError(msg.error));
    return;
  }

  successCallback();
}

function _typeMismatchException() {
  throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
}

function _requestRouteToHost(
    networkType, domainName, networkSuccessCallback, errorCallback) {
  if (typeof networkType !== 'string' || !(networkType in NetworkType))
    _typeMismatchException();

  if (typeof domainName !== 'string')
    _typeMismatchException();

  if (typeof networkSuccessCallback !== 'object')
    _typeMismatchException();

  if (networkSuccessCallback == null)
    _typeMismatchException();

  if (networkSuccessCallback.hasOwnProperty('onsuccess') &&
      typeof networkSuccessCallback.onsuccess !== 'function')
    _typeMismatchException();

  if (networkSuccessCallback.hasOwnProperty('ondisconnected') &&
      typeof networkSuccessCallback.ondisconnected !== 'function')
    _typeMismatchException();

  if (arguments.length == 4 && typeof errorCallback !== 'function')
    _typeMismatchException();

  var id = (_next_reply_id++).toString();
  _callbacks[id] = [networkSuccessCallback, errorCallback];

  extension.postMessage(JSON.stringify({
    'cmd': 'requestRouteToHost',
    'network_type': NetworkType[networkType],
    'domain_name': domainName,
    'reply_id': id
  }));
}

function _releaseRouteToHost(
    networkType, domainName, successCallback, errorCallback) {
  if (typeof networkType !== 'string' || !(networkType in NetworkType))
    _typeMismatchException();

  if (typeof domainName !== 'string')
    _typeMismatchException();

  if (typeof successCallback !== 'function')
    _typeMismatchException();

  if (arguments.length == 4 && typeof errorCallback !== 'function')
    _typeMismatchException();

  var id = (_next_reply_id++).toString();
  _callbacks[id] = [successCallback, errorCallback];

  extension.postMessage(JSON.stringify({
    'cmd': 'releaseRouteToHost',
    'network_type': NetworkType[networkType],
    'domain_name': domainName,
    'reply_id': id
  }));
}

extension.setMessageListener(_messageListener);

exports.requestRouteToHost = _requestRouteToHost;
exports.releaseRouteToHost = _releaseRouteToHost;

// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var callbacks = {};
var callbackId = 0;
var cbKey = '_callback';

function setupCallback(callback) {
  var id = ++callbackId;
  callbacks[id] = callback;
  return id;
}

// This callback will dispatch message to different handlers by callbackId.
extension.setMessageListener(function(msg) {
  var m = JSON.parse(msg);
  var callback = callbacks[m[cbKey]];
  delete callbacks[m[cbKey]];
  callback(m);
});

// Post async message to extension with callbackId saved. The extension will return
// a message with the same callbackId to the callback set in setMessageListener.
var postMessage = function(msg, callbackId) {
  msg[cbKey] = callbackId;
  extension.postMessage(JSON.stringify(msg));
};

var sendSyncMessage = function(msg) {
  return JSON.parse(extension.internal.sendSyncMessage(JSON.stringify(msg)));
};

function ApplicationInformation(json) {
  for (var field in json) {
    var val = json[field];
    if (field === 'installDate')
      val = new Date(val * 1000);
    Object.defineProperty(this, field, { writable: false, enumerable: true, value: val });
  }
}

exports.getAppInfo = function(appId) {
  if (typeof appId !== 'string' && appId != undefined)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var result = sendSyncMessage({ cmd: 'GetAppInfo', id: appId });
  if (!result.error)
    return new ApplicationInformation(result);

  if (result.error === 'appinfo')
    throw new tizen.WebAPIException(tizen.WebAPIException.NOT_FOUND_ERR);
  else
    throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);
};

exports.getAppsInfo = function(onsuccess, onerror) {
  if ((typeof onsuccess !== 'function') ||
      (onerror && typeof onerror !== 'function'))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var callbackId = setupCallback(function(result) {
    if (result.error)
      return onerror(new tizen.WebAPIError(tizen.WebAPIException.UNKNOWN_ERR));

    var appsInfo = [];
    for (var i = 0, len = result.data.length; i < len; ++i)
      appsInfo.push(new ApplicationInformation(result.data[i]));
    return onsuccess(appsInfo);
  });

  var msg = { cmd: 'GetAppsInfo' };
  postMessage(msg, callbackId);
};

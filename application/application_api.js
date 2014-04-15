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
  console.log(msg);
  var m = JSON.parse(msg);
  var callback = callbacks[m[cbKey]];
  delete callbacks[m[cbKey]];
  callback(m);
});

// Post async message to extension with callbackId saved. The extension will return
// a message with the same callbackId to the callback set in setMessageListener.
function postMessage(msg, callbackId) {
  msg[cbKey] = callbackId;
  extension.postMessage(JSON.stringify(msg));
}

function sendSyncMessage(msg) {
  return JSON.parse(extension.internal.sendSyncMessage(JSON.stringify(msg)));
}

function defineReadOnlyProperty(object, key, value) {
  Object.defineProperty(object, key, {
    enumerable: true,
    writable: false,
    value: value
  });
}

// Define Application interface, the getRequestedAppControl method will not be
// implemented ATM.
function Application(appInfo, contextId) {
  defineReadOnlyProperty(this, 'appInfo', appInfo);
  defineReadOnlyProperty(this, 'contextId', contextId);
}

Application.prototype.exit = function() {
  var result = sendSyncMessage({ cmd: 'ExitCurrentApp' });
  if (result.error)
    return new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);
};

Application.prototype.hide = function() {
  var result = sendSyncMessage({ cmd: 'HideCurrentApp' });
  if (result.error)
    return new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);
};

// ApplicationContext interface.
function ApplicationContext(json) {
  defineReadOnlyProperty(this, 'id', json.id);
  defineReadOnlyProperty(this, 'appId', json.appId);
}

// ApplicationInformation interface.
function ApplicationInformation(json) {
  for (var field in json) {
    var val = json[field];
    if (field === 'installDate')
      val = new Date(val * 1000);
    defineReadOnlyProperty(this, field, val);
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
    if (result.error != undefined) {
      if (!onerror)
        return;
      return onerror(new tizen.WebAPIError(tizen.WebAPIException.UNKNOWN_ERR));
    }

    var appsInfo = [];
    for (var i = 0, len = result.data.length; i < len; ++i)
      appsInfo.push(new ApplicationInformation(result.data[i]));
    return onsuccess(appsInfo);
  });

  var msg = { cmd: 'GetAppsInfo' };
  postMessage(msg, callbackId);
};

exports.getAppContext = function(contextId) {
  if (contextId && typeof contextId !== 'string')
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var result = sendSyncMessage({ cmd: 'GetAppContext', id: contextId});
  if (result.error)
    throw new tizen.WebAPIException(result.error.code);

  return new ApplicationContext(result.data);
};

exports.getAppsContext = function(onsuccess, onerror) {
  if ((typeof onsuccess !== 'function') ||
      (onerror && typeof onerror !== 'function'))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var callbackId = setupCallback(function(result) {
    if (result.error != undefined) {
      if (!onerror)
        return;
      return onerror(new tizen.WebAPIError(tizen.WebAPIException.UNKNOWN_ERR));
    }

    var contexts = [];
    for (var i = 0, len = result.data.length; i < len; ++i)
      contexts.push(new ApplicationContext(result.data[i]));
    return onsuccess(contexts);
  });

  var msg = { cmd: 'GetAppsContext' };
  postMessage(msg, callbackId);
};

exports.getCurrentApplication = function() {
  var result = sendSyncMessage({ cmd: 'GetCurrentApp' });
  if (result.appInfo.error || result.appContext.error)
    throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);

  var appInfo = new ApplicationInformation(JSON.parse(result.appInfo));
  return new Application(appInfo, JSON.parse(result.appContext).id);
};

// TODO(xiang): add privilege level check for this method.
exports.kill = function(contextId, onsuccess, onerror) {
  if ((typeof contextId !== 'string') ||
      (onsuccess && typeof onsuccess !== 'function') ||
      (onerror && typeof onerror !== 'function'))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var callbackId = setupCallback(function(result) {
    if (result.error) {
      if (!onerror)
        return;
      return onerror(new tizen.WebAPIError(result.error.code));
    }
    return onsuccess();
  });

  var msg = { cmd: 'KillApp', id: contextId };
  postMessage(msg, callbackId);
};

// TODO(xiang): add privilege level check for this method.
exports.launch = function(appId, onsuccess, onerror) {
  if ((typeof appId !== 'string') ||
      (onsuccess && typeof onsuccess !== 'function') ||
      (onerror && typeof onerror !== 'function'))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var callbackId = setupCallback(function(result) {
    if (result.error) {
      if (!onerror)
        return;
      return onerror(new tizen.WebAPIError(result.error.code));
    }
    return onsuccess();
  });

  var msg = { cmd: 'LaunchApp', id: appId };
  postMessage(msg, callbackId);
};

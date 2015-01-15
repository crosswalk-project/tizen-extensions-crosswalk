// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
var PackageEventState = {
  'STARTED': 0,
  'PROCESSING': 1,
  'COMPLETED': 2,
  'FAILED': 3
};

var PackageEventType = {
  'INSTALL': 0,
  'UNINSTALL': 1,
  'UPDATE': 2
};

var asyncCallbacks = {
  _next_id: 0,
  _callbacks: {},
  key: '_callback',
  infoEvent: '_pkgInfoEventCallback',

  setup: function(callback) {
    var id = ++this._next_id;
    this._callbacks[id] = callback;
    return id;
  },

  dispatch: function(m) {
    var id = m[this.key];
    var callback = this._callbacks[id];
    callback.call(null, m);
    if (m.error != null || m.cmd == PackageEventState.COMPLETED ||
        m.cmd == PackageEventState.FAILED)
      delete this._callbacks[id];
  }
};

extension.setMessageListener(function(msg) {
  var m = JSON.parse(msg);
  if (typeof m[asyncCallbacks.key] === 'number') {
    asyncCallbacks.dispatch(m);
  } else if (m[asyncCallbacks.key] === asyncCallbacks.infoEvent &&
             exports.changeListener != null) {
    if (m.eventType == PackageEventType.INSTALL) {
      var pkgInfo = new PackageInformation(m.data);
      exports.changeListener.oninstalled(pkgInfo);
    }
    if (m.eventType == PackageEventType.UPDATE) {
      var pkgInfo = new PackageInformation(m.data);
      exports.changeListener.onupdated(m.data);
    }
    if (m.eventType == PackageEventType.UNINSTALL) {
      exports.changeListener.onuninstalled(m.data);
    }
  } else {
    console.error('unexpected message received' + msg);
  }
});

function postMessage(msg, callbackId) {
  msg[asyncCallbacks.key] = callbackId;
  extension.postMessage(JSON.stringify(msg));
}

function sendSyncMessage(msg) {
  return JSON.parse(extension.internal.sendSyncMessage(JSON.stringify(msg)));
}

function PackageManager() {
  this.changeListener = null;
}

function defineReadOnlyProperty(object, key, value) {
  Object.defineProperty(object, key, {
    enumerable: true,
    writable: false,
    value: value
  });
}

// PackageInformation interface.
function PackageInformation(json) {
  for (var field in json) {
    var val = json[field];
    if (field === 'installDate')
      val = new Date(val * 1000);
    defineReadOnlyProperty(this, field, val);
  }
}

PackageManager.prototype.install = function(path, onsuccess, onerror) {
  if (typeof path !== 'string' ||
      onsuccess == null ||
      onsuccess.onprogress == null || onsuccess.oncomplete == null ||
      onerror !== undefined && typeof onerror !== 'function')
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var callbackId = asyncCallbacks.setup(function(result) {
    if (result.error != null && typeof onerror === 'function') {
      onerror(new tizen.WebAPIError(result.errorCode));
    } else if (result.cmd == PackageEventState.COMPLETED &&
               typeof onsuccess.oncomplete !== 'undefined') {
      onsuccess.oncomplete(result.id);
    } else if (typeof onsuccess.onprogress !== 'undefined') {
      onsuccess.onprogress(result.id, result.progress);
    }
  });

  var msg = { cmd: 'PackageManager.install', id: path };
  postMessage(msg, callbackId);
};

PackageManager.prototype.uninstall = function(id, onsuccess, onerror) {
  if (typeof id !== 'string' ||
      onsuccess == null ||
      onsuccess.onprogress == null || onsuccess.oncomplete == null ||
      onerror !== undefined && typeof onerror !== 'function')
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var callbackId = asyncCallbacks.setup(function(result) {
    if (result.error != null && typeof onerror === 'function') {
      return onerror(new tizen.WebAPIError(result.error));
    } else if (result.cmd == PackageEventState.COMPLETED &&
               typeof onsuccess.oncomplete !== 'undefined') {
      onsuccess.oncomplete(result.id);
    } else if (typeof onsuccess.onprogress !== 'undefined') {
      onsuccess.onprogress(result.id, result.progress);
    }
  });

  var msg = { cmd: 'PackageManager.uninstall', id: id };
  postMessage(msg, callbackId);
};

PackageManager.prototype.getPackageInfo = function(pkgId) {
  if (typeof pkgId !== 'string' && pkgId != undefined)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var result = sendSyncMessage({ cmd: 'PackageManager.getPackageInfo', id: pkgId });
  if (result.error != null)
    throw new tizen.WebAPIException(result.error);

  return new PackageInformation(result.data);
};

PackageManager.prototype.getPackagesInfo = function(onsuccess, onerror) {
  if (typeof onsuccess !== 'function' ||
      onerror !== undefined && typeof onerror !== 'function')
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var callbackId = asyncCallbacks.setup(function(result) {
    if (result.error != null && typeof onerror === 'function') {
      return onerror(new tizen.WebAPIError(result.error));
    }

    var pkgsInfo = [];
    for (var i = 0, len = result.data.length; i < len; ++i)
      pkgsInfo.push(new PackageInformation(result.data[i]));
    return onsuccess(pkgsInfo);
  });

  var msg = { cmd: 'PackageManager.getPackagesInfo' };
  postMessage(msg, callbackId);
};

PackageManager.prototype.setPackageInfoEventListener = function(listener) {
  if (typeof listener !== 'object' ||
      typeof listener.oninstalled !== 'function' ||
      typeof listener.onupdated !== 'function' ||
      typeof listener.onuninstalled !== 'function')
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  this.changeListener = listener;
  sendSyncMessage({cmd: 'PackageManager.setPackageInfoEventListener'});
};

PackageManager.prototype.unsetPackageInfoEventListener = function() {
  this.changeListener = null;
  sendSyncMessage({cmd: 'PackageManager.unsetPackageInfoEventListener'});
};

exports = new PackageManager();

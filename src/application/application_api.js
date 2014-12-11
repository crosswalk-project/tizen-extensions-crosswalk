// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var asyncCallbacks = {
  _next_id: 0,
  _callbacks: {},
  key: '_callback',

  // Return a callback ID number which will be contained by the native message.
  setup: function(callback) {
    var id = ++this._next_id;
    this._callbacks[id] = callback;
    return id;
  },

  dispatch: function(m) {
    var id = m[this.key];
    var callback = this._callbacks[id];
    callback.call(null, m);
    delete this._callbacks[id];
  }
};

var appInfoEventCallbacks = {
  _next_id: 0,
  _callbacks: [],
  key: '_appInfoEventCallback',

  // Return a callback ID number which can be unregistered later.
  addCallback: function(callback) {
    if (!this._callbacks.length) {
      var result = sendSyncMessage({ cmd: 'RegisterAppInfoEvent' });
      if (result.error != null)
        throw new tizen.WebAPIException(result.error);
    }

    var id = ++this._next_id;
    this._callbacks.push({func: callback, id: id});
    return id;
  },

  removeCallback: function(id) {
    for (var i = 0, len = this._callbacks.length; i < len; i++) {
      if (id === this._callbacks[i].id)
        break;
    }
    if (i == len)
      throw new tizen.WebAPIException(tizen.WebAPIException.NOT_FOUND_ERR);

    this._callbacks.splice(i, 1);
    if (!this._callbacks.length) {
      var result = sendSyncMessage({ cmd: 'UnregisterAppInfoEvent' });
      if (result.error != null)
        throw new tizen.WebAPIException(result.error);
    }
  },

  dispatch: function(m) {
    var callbacks = this._callbacks.slice();
    for (var i = 0, len = callbacks.length; i < len; i++)
      callbacks[i].func.call(null, m);
  }
};

extension.setMessageListener(function(msg) {
  var m = JSON.parse(msg);
  if (typeof m[asyncCallbacks.key] === 'number') {
    asyncCallbacks.dispatch(m);
  } else {
    if (m[asyncCallbacks.key] === appInfoEventCallbacks.key)
      appInfoEventCallbacks.dispatch(m);
    else
      console.error('unexpected message received' + msg);
  }
});

// Post async message to extension with callbackId saved. The extension will return
// a message with the same callbackId to the callback set in setMessageListener.
function postMessage(msg, callbackId) {
  msg[asyncCallbacks.key] = callbackId;
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
  if (result.error != null)
    throw new tizen.WebAPIException(result.error);
};

Application.prototype.hide = function() {
  var result = sendSyncMessage({ cmd: 'HideCurrentApp' });
  if (result.error != null)
    throw new tizen.WebAPIException(result.error);
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

// ApplicationMetaData interface.
function ApplicationMetaData(json) {
  defineReadOnlyProperty(this, 'key', json.key);
  defineReadOnlyProperty(this, 'value', json.value);
}

exports.getAppInfo = function(appId) {
  if (typeof appId !== 'string' && appId != undefined)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var result = sendSyncMessage({ cmd: 'GetAppInfo', id: appId });
  if (result.error != null)
    throw new tizen.WebAPIException(result.error);
  return new ApplicationInformation(result.data);
};

exports.getAppsInfo = function(onsuccess, onerror) {
  if ((typeof onsuccess !== 'function') ||
      (arguments.length > 1 && (onerror === undefined || typeof onerror !== 'function')))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var callbackId = asyncCallbacks.setup(function(result) {
    if (result.error != null) {
      if (!onerror)
        return;
      return onerror(new tizen.WebAPIError(result.error));
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
  if (contextId && !/^\d+$/.test(contextId))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var result = sendSyncMessage({ cmd: 'GetAppContext', id: contextId});
  if (result.error)
    throw new tizen.WebAPIException(result.error);

  return new ApplicationContext(result.data);
};

exports.getAppsContext = function(onsuccess, onerror) {
  if ((typeof onsuccess !== 'function') ||
      (arguments.length > 1 && (onerror === undefined || typeof onerror !== 'function')))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var callbackId = asyncCallbacks.setup(function(result) {
    if (result.error != null) {
      if (!onerror)
        return;
      return onerror(new tizen.WebAPIError(result.error));
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
  if (result.error)
    throw new tizen.WebAPIException(result.error);

  var appInfo = new ApplicationInformation(result.data.appInfo);
  return new Application(appInfo, result.data.appContext.id);
};

exports.kill = function(contextId, onsuccess, onerror) {
  if ((!/^\d+$/.test(contextId)) ||
      (onsuccess && typeof onsuccess !== 'function') ||
      (onerror && typeof onerror !== 'function'))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var callbackId = asyncCallbacks.setup(function(result) {
    if (result.error != null) {
      if (!onerror)
        return;
      return onerror(new tizen.WebAPIError(result.code));
    }
    return onsuccess();
  });

  var msg = { cmd: 'KillApp', id: contextId };
  postMessage(msg, callbackId);
};

exports.launch = function(appId, onsuccess, onerror) {
  if ((typeof appId !== 'string') ||
      (onsuccess && typeof onsuccess !== 'function') ||
      (onerror && typeof onerror !== 'function'))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var callbackId = asyncCallbacks.setup(function(result) {
    if (result.error != null) {
      if (!onerror)
        return;
      return onerror(new tizen.WebAPIError(result.error));
    }
    return onsuccess();
  });

  var msg = { cmd: 'LaunchApp', id: appId };
  postMessage(msg, callbackId);
};

exports.addAppInfoEventListener = function(eventCallback) {
  if (typeof eventCallback !== 'object' ||
      typeof eventCallback.oninstalled !== 'function' ||
      typeof eventCallback.onupdated !== 'function' ||
      typeof eventCallback.onuninstalled !== 'function')
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var watchId = appInfoEventCallbacks.addCallback(function(result) {
    if (result.installed) {
      result.installed.forEach(function(appInfo) {
        var appInfo = new ApplicationInformation(appInfo);
        eventCallback.oninstalled(appInfo);
      });
    }
    if (result.updated) {
      result.updated.forEach(function(appInfo) {
        var appInfo = new ApplicationInformation(appInfo);
        eventCallback.onupdated(appInfo);
      });
    }
    if (result.uninstalled) {
      result.uninstalled.forEach(function(appId) {
        eventCallback.onuninstalled(appId);
      });
    }
  });

  return watchId;
};

exports.removeAppInfoEventListener = function(watchId) {
  if (typeof watchId !== 'number')
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  appInfoEventCallbacks.removeCallback(watchId);
};

exports.getAppMetaData = function(appId) {
  if (typeof appId !== 'string' && appId != undefined)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var result = sendSyncMessage({ cmd: 'GetAppMetaData', id: appId });
  if (result.error != null)
    throw new tizen.WebAPIException(result.error);

  var data = [];
  for (var i = 0, len = result.data.length; i < len; ++i)
    data.push(new ApplicationMetaData(result.data[i]));
  return data;
};

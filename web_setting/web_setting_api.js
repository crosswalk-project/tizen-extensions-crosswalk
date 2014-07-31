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

function defineReadOnlyProperty(object, key, value) {
  Object.defineProperty(object, key, {
    enumerable: true,
    writable: false,
    value: value
  });
}

exports.setUserAgentString = function(userAgent,
                                      successCallback, errorCallback) {
  if (arguments.length > 0 && typeof userAgent !== 'string' ||
      arguments.length > 1 && (successCallback !== null &&
      typeof successCallback !== 'function') || arguments.length > 2 &&
      (errorCallback !== null && typeof errorCallback !== 'function')) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  var callbackId = asyncCallbacks.setup(function(result) {
    if (result.error !== null) {
      if (!errorCallback) {
        return;
      }
      return errorCallback(new tizen.WebAPIError(result.error));
    }
    if (successCallback !== null) {
      return successCallback(result.data);
    }
    return;
  });

  var msg = { cmd: 'SetUserAgentString', userAgentStr: userAgent };
  postMessage(msg, callbackId);
};

exports.removeAllCookies = function(successCallback, errorCallback) {
  if (arguments.length > 0 && (successCallback !== null &&
      typeof successCallback !== 'function') || arguments.length > 1 &&
      (errorCallback !== null && typeof errorCallback !== 'function')) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  var callbackId = asyncCallbacks.setup(function(result) {
    if (result.error !== null) {
      if (!errorCallback) {
        return;
      }
      return errorCallback(new tizen.WebAPIError(result.error));
    }
    if (successCallback !== null) {
      return successCallback(result.data);
    }
    return;
  });

  var msg = { cmd: 'RemoveAllCookies' };
  postMessage(msg, callbackId);
};

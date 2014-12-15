// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var repliedMsg;
var currentUID = 0;
var requests = {};
var startListeners = [];
var networkTypeCallbacks = [];
var mimeTypeCallbacks = [];
var httpHeaderCallbacks = [];

var postMessage = function(msg) {
  extension.postMessage(JSON.stringify(msg));
};

var asValidString = function(o) {
  return (typeof o == 'string') ? o : '';
};

var ensureType = function(o, expected) {
  if (typeof o != expected || o === undefined) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }
};

var ensureHas = function(o) {
  if (typeof o == 'undefined') {
    throw new tizen.WebAPIException(tizen.WebAPIException.NOT_FOUND_ERR);
  }
};

var getNetworkTypeThrowsError = function(networkType) {
  if (typeof networkType == 'undefined') {
    return 'ALL';
  } else if (networkType in AllowDownloadOnNetworkType) {
    return networkType;
  } else {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }
};

var errorMap = {
  'DOWNLOAD_ERROR_NONE': {
    code: tizen.WebAPIException.NOT_SUPPORTED_ERR,
    name: 'NotSupportedError',
    message: 'Download successful'
  },
  'DOWNLOAD_UNKNOWN_ERROR': {
    code: tizen.WebAPIException.NOT_SUPPORTED_ERR,
    name: 'NotSupportedError',
    message: 'Unknown error'
  },
  'DOWNLOAD_ERROR_INVALID_PARAMETER': {
    code: tizen.WebAPIException.NOT_SUPPORTED_ERR,
    name: 'NotSupportedError',
    message: 'Invalid parameter'
  },
  'DOWNLOAD_ERROR_FIELD_NOT_FOUND': {
    code: tizen.WebAPIException.NOT_SUPPORTED_ERR,
    name: 'NotSupportedError',
    message: 'Specified field not found'
  },
  'DOWNLOAD_ERROR_TOO_MANY_DOWNLOADS': {
    code: tizen.WebAPIException.NOT_SUPPORTED_ERR,
    name: 'NotSupportedError',
    message: 'Full of available downloading items from server'
  },
  'DOWNLOAD_ERROR_FILE_ALREADY_EXISTS': {
    code: tizen.WebAPIException.NOT_SUPPORTED_ERR,
    name: 'NotSupportedError',
    message: 'It is failed to rename the downloaded file'
  },
  'DOWNLOAD_ERROR_INVALID_URL': {
    code: tizen.WebAPIException.SYNTAX_ERR,
    name: 'SyntaxError',
    message: 'Invalid URL'
  },
  'DOWNLOAD_ERROR_INVALID_DESTINATION': {
    code: tizen.WebAPIException.SYNTAX_ERR,
    name: 'SyntaxError',
    message: 'Invalid destination'
  },
  'DOWNLOAD_ERROR_INVALID_STATE': {
    code: tizen.WebAPIException.INVALID_STATE_ERR,
    name: 'InvalidStateError',
    message: 'Invalid state'
  },
  'DOWNLOAD_ERROR_ALREADY_COMPLETED': {
    code: tizen.WebAPIException.INVALID_STATE_ERR,
    name: 'InvalidStateError',
    message: 'The download is already completed'
  },
  'DOWNLOAD_ERROR_CANNOT_RESUME': {
    code: tizen.WebAPIException.NOT_SUPPORTED_ERR,
    name: 'NotSupportedError',
    message: 'It cannot resume'
  },
  'DOWNLOAD_ERROR_IO_ERROR': {
    code: tizen.WebAPIException.INVALID_ACCESS_ERR,
    name: 'InvalidAccessError',
    message: 'Internal I/O error'
  },
  'DOWNLOAD_ERROR_OUT_OF_MEMORY': {
    code: tizen.WebAPIException.QUOTA_EXCEEDED_ERR,
    name: 'QuotaExceededError',
    message: 'Out of memory'
  },
  'DOWNLOAD_ERROR_NO_SPACE': {
    code: tizen.WebAPIException.QUOTA_EXCEEDED_ERR,
    name: 'QuotaExceededError',
    message: 'No space left on device'
  },
  'DOWNLOAD_ERROR_NETWORK_UNREACHABLE': {
    code: tizen.WebAPIException.NETWORK_ERR,
    name: 'NetworkError',
    message: 'Network is unreachable'
  },
  'DOWNLOAD_ERROR_CONNECTION_TIMED_OUT': {
    code: tizen.WebAPIException.NETWORK_ERR,
    name: 'NetworkError',
    message: 'HTTP session timeout'
  },
  'DOWNLOAD_ERROR_CONNECTION_FAILED': {
    code: tizen.WebAPIException.NETWORK_ERR,
    name: 'NetworkError',
    message: 'Connection failed'
  },
  'DOWNLOAD_ERROR_REQUEST_TIMEOUT': {
    code: tizen.WebAPIException.NETWORK_ERR,
    name: 'NetworkError',
    message: 'There are no action after client create a download id'
  },
  'DOWNLOAD_ERROR_RESPONSE_TIMEOUT': {
    code: tizen.WebAPIException.NETWORK_ERR,
    name: 'NetworkError',
    message: 'It does not call start API in some time although the download is created'
  },
  'DOWNLOAD_ERROR_TOO_MANY_REDIRECTS': {
    code: tizen.WebAPIException.NETWORK_ERR,
    name: 'NetworkError',
    message: 'In case of too may redirects from http response header'
  },
  'DOWNLOAD_ERROR_UNHANDLED_HTTP_CODE': {
    code: tizen.WebAPIException.NETWORK_ERR,
    name: 'NetworkError',
    message: 'The download cannot handle the HTTP status value'
  },
  'DOWNLOAD_ERROR_SYSTEM_DOWN': {
    code: tizen.WebAPIException.NOT_SUPPORTED_ERR,
    name: 'NotSupportedError',
    message: 'There are no response from client after rebooting download daemon'
  },
  'DOWNLOAD_ERROR_NO_DATA': {
    code: tizen.WebAPIException.NOT_SUPPORTED_ERR,
    name: 'NotSupportedError',
    message: 'No data because the set API is not called'
  },
  'DOWNLOAD_ERROR_ID_NOT_FOUND': {
    code: tizen.WebAPIException.NOT_SUPPORTED_ERR,
    name: 'NotSupportedError',
    message: 'The download id is not existed in download service module'
  }
};

var AllowDownloadOnNetworkType = {
  'ALL': 0,
  'CELLULAR': 1,
  'WIFI': 2
};

extension.setMessageListener(function(msg) {
  var m = JSON.parse(msg);
  var id = parseInt(m.uid);
  if (isNaN(id) || typeof startListeners[id] === 'undefined') {
    return;
  } else if (m.cmd == 'DownloadReplyProgress') {
    if (typeof startListeners[id].onprogress !== 'undefined') {
      var receivedSize = parseInt(m.receivedSize);
      var totalSize = parseInt(m.totalSize);
      startListeners[id].onprogress(id, receivedSize, totalSize);
    }
  } else if (m.cmd == 'DownloadReplyComplete') {
    if (typeof startListeners[id].oncompleted !== 'undefined') {
      startListeners[id].oncompleted(id, m.fullPath);
    }
  } else if (m.cmd == 'DownloadReplyPause') {
    if (typeof startListeners[id].onpaused !== 'undefined') {
      startListeners[id].onpaused(id);
    }
  } else if (m.cmd == 'DownloadReplyCancel') {
    if (typeof startListeners[id].oncanceled !== 'undefined') {
      startListeners[id].oncanceled(id);
    }
  } else if (m.cmd == 'DownloadReplyNetworkType') {
    networkTypeCallbacks[id](m.networkType);
  } else if (m.cmd == 'DownloadReplyMIMEType') {
    mimeTypeCallbacks[id](m.mimeType);
  } else if (m.cmd == 'DownloadReplyFail') {
    startListeners[id].onfailed(id,
        new tizen.WebAPIError(errorMap[m.errorCode].code,
                              errorMap[m.errorCode].message,
                              errorMap[m.errorCode].name));
  }
});

tizen.DownloadRequest = function(url, destination, fileName, networkType) {
  Object.defineProperty(this, 'networkType', {
    get: function() { return this.networkTypeValue; },
    set: function(type) {
      if (type === null || type in AllowDownloadOnNetworkType) {
        this.networkTypeValue = type;
      }
    }
  });

  var url_;
  Object.defineProperty(this, 'url', {
    get: function() { return this.url_; },
    set: function(value) {
      if (value != null) {
        this.url_ = value;
      }
    }
  });
  this.url_ = url;

  if (!(this instanceof tizen.DownloadRequest)) {
    throw new TypeError;
  }
  this.uid = ++currentUID;
  this.destination = asValidString(destination);
  this.fileName = asValidString(fileName);
  this.networkType = getNetworkTypeThrowsError(networkType);
  this.httpHeader = {};
};

exports.start = function(request, listener) {
  if (!(request instanceof tizen.DownloadRequest)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }
  requests[request.uid] = request;
  if (arguments.length > 1 && listener !== null) {
    ensureType(listener, 'object');
    exports.setListener(request.uid, listener);
  }
  postMessage({
    'cmd': 'DownloadStart',
    'url': request.url,
    'uid': request.uid,
    'destination': request.destination,
    'fileName': request.fileName,
    'networkType': request.networkType,
    'httpHeader': request.httpHeader
  });
  return request.uid;
};

exports.setListener = function(downloadId, listener) {
  ensureType(downloadId, 'number');
  ensureType(listener, 'object');
  if (listener === null) { // null is also an object, so we need double check
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }
  for (var property in listener) {
    ensureType(listener[property], 'function');
  }
  ensureHas(requests[downloadId]);
  startListeners[downloadId] = listener;
};

exports.pause = function(downloadId) {
  ensureType(downloadId, 'number');
  ensureHas(requests[downloadId]);
  postMessage({
    'cmd': 'DownloadPause',
    'uid': downloadId
  });
};

exports.resume = function(downloadId) {
  ensureType(downloadId, 'number');
  ensureHas(requests[downloadId]);
  postMessage({
    'cmd': 'DownloadResume',
    'uid': downloadId
  });
};

exports.cancel = function(downloadId) {
  ensureType(downloadId, 'number');
  ensureHas(requests[downloadId]);
  postMessage({
    'cmd': 'DownloadCancel',
    'uid': downloadId
  });
};

exports.getDownloadRequest = function(downloadId) {
  ensureType(downloadId, 'number');
  ensureHas(requests[downloadId]);
  return requests[downloadId];
};

exports.getNetworkType = function(downloadId, callback) {
  ensureType(downloadId, 'number');
  ensureType(callback, 'function');
  ensureHas(requests[downloadId]);
  networkTypeCallbacks[downloadId] = callback;
  postMessage({
    'cmd': 'DownloadGetNetworkType',
    'uid': downloadId
  });
};

exports.getMIMEType = function(downloadId) {
  ensureType(downloadId, 'number');
  ensureHas(requests[downloadId]);
  var reply = JSON.parse(_sendSyncMessage({
    'cmd': 'DownloadGetMIMEType',
    'uid': downloadId
  }));
  if (reply['error'] != 'DOWNLOAD_ERROR_NONE') {
    switch (reply['error']) {
      case 'DOWNLOAD_ERROR_INVALID_PARAMETER':
        throw new tizen.WebAPIException(tizen.WebAPIException.SYNTAX_ERR);
        break;
      case 'DOWNLOAD_ERROR_ID_NOT_FOUND':
        throw new tizen.WebAPIException(tizen.WebAPIException.NOT_FOUND_ERR);
        break;
      default:
        throw new tizen.WebAPIException(tizen.WebAPIException.NOT_SUPPORTED_ERR);
    }
  } else {
    delete reply['error'];
    return reply['mimeType'];
  }
};

exports.getState = function(downloadId) {
  ensureType(downloadId, 'number');
  ensureHas(requests[downloadId]);
  var reply = JSON.parse(_sendSyncMessage({
    'cmd': 'DownloadGetState',
    'uid': downloadId
  }));
  if (reply['error'] != 'DOWNLOAD_ERROR_NONE') {
    switch (reply['error']) {
      case 'DOWNLOAD_ERROR_INVALID_PARAMETER':
        // TODO(hdq): Spec said to throw 'InvalidValuesError' here,
        // but there is no such value in WebAPIException
        throw new tizen.WebAPIException(tizen.WebAPIException.SYNTAX_ERR);
        break;
      case 'DOWNLOAD_ERROR_ID_NOT_FOUND':
        throw new tizen.WebAPIException(tizen.WebAPIException.NOT_FOUND_ERR);
        break;
      default:
        throw new tizen.WebAPIException(tizen.WebAPIException.NOT_SUPPORTED_ERR);
    }
  } else {
    delete reply['error'];
    return reply['state'];
  }
};

var _sendSyncMessage = function(msg) {
  return extension.internal.sendSyncMessage(JSON.stringify(msg));
};

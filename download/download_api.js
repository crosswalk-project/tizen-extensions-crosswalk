// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var repliedMsg;
var currentUID = 0;
var startListeners = [];

var postMessage = function(msg) {
  extension.postMessage(JSON.stringify(msg));
};

var errorMap = {
  "DOWNLOAD_ERROR_NONE" : {
    code : tizen.WebAPIException.NOT_SUPPORTED_ERR,
    name : "NotSupportedError",
    message : "Download successful",
  },
  "DOWNLOAD_UNKNOWN_ERROR" : {
    code : tizen.WebAPIException.NOT_SUPPORTED_ERR,
    name : "NotSupportedError",
    message : "Unknown error",
  },
  "DOWNLOAD_ERROR_INVALID_PARAMETER" : {
    code : tizen.WebAPIException.NOT_SUPPORTED_ERR,
    name : "NotSupportedError",
    message : "Invalid parameter",
  },
  "DOWNLOAD_ERROR_FIELD_NOT_FOUND" : {
    code : tizen.WebAPIException.NOT_SUPPORTED_ERR,
    name : "NotSupportedError",
    message : "Specified field not found",
  },
  "DOWNLOAD_ERROR_TOO_MANY_DOWNLOADS" : {
    code : tizen.WebAPIException.NOT_SUPPORTED_ERR,
    name : "NotSupportedError",
    message : "Full of available downloading items from server",
  },
  "DOWNLOAD_ERROR_FILE_ALREADY_EXISTS" : {
    code : tizen.WebAPIException.NOT_SUPPORTED_ERR,
    name : "NotSupportedError",
    message : "It is failed to rename the downloaded file",
  },
  "DOWNLOAD_ERROR_INVALID_URL" : {
    code : tizen.WebAPIException.SYNTAX_ERR,
    name : "SyntaxError",
    message : "Invalid URL",
  },
  "DOWNLOAD_ERROR_INVALID_DESTINATION" : {
    code : tizen.WebAPIException.SYNTAX_ERR,
    name : "SyntaxError",
    message : "Invalid destination",
  },
  "DOWNLOAD_ERROR_INVALID_STATE" : {
    code : tizen.WebAPIException.INVALID_STATE_ERR,
    name : "InvalidStateError",
    message : "Invalid state",
  },
  "DOWNLOAD_ERROR_ALREADY_COMPLETED" : {
    code : tizen.WebAPIException.INVALID_STATE_ERR,
    name : "InvalidStateError",
    message : "The download is already completed",
  },
  "DOWNLOAD_ERROR_CANNOT_RESUME" : {
    code : tizen.WebAPIException.NOT_SUPPORTED_ERR,
    name : "NotSupportedError",
    message : "It cannot resume",
  },
  "DOWNLOAD_ERROR_IO_ERROR" : {
    code : tizen.WebAPIException.INVALID_ACCESS_ERR,
    name : "InvalidAccessError",
    message : "Internal I/O error",
  },
  "DOWNLOAD_ERROR_OUT_OF_MEMORY" : {
    code : tizen.WebAPIException.QUOTA_EXCEEDED_ERR,
    name : "QuotaExceededError",
    message : "Out of memory",
  },
  "DOWNLOAD_ERROR_NO_SPACE" : {
    code : tizen.WebAPIException.QUOTA_EXCEEDED_ERR,
    name : "QuotaExceededError",
    message : "No space left on device",
  },
  "DOWNLOAD_ERROR_NETWORK_UNREACHABLE" : {
    code : tizen.WebAPIException.NETWORK_ERR,
    name : "NetworkError",
    message : "Network is unreachable",
  },
  "DOWNLOAD_ERROR_CONNECTION_TIMED_OUT" : {
    code : tizen.WebAPIException.NETWORK_ERR,
    name : "NetworkError",
    message : "HTTP session timeout",
  },
  "DOWNLOAD_ERROR_CONNECTION_FAILED" : {
    code : tizen.WebAPIException.NETWORK_ERR,
    name : "NetworkError",
    message : "Connection failed",
  },
  "DOWNLOAD_ERROR_REQUEST_TIMEOUT" : {
    code : tizen.WebAPIException.NETWORK_ERR,
    name : "NetworkError",
    message : "There are no action after client create a download id",
  },
  "DOWNLOAD_ERROR_RESPONSE_TIMEOUT" : {
    code : tizen.WebAPIException.NETWORK_ERR,
    name : "NetworkError",
    message : "It does not call start API in some time although the download is created",
  },
  "DOWNLOAD_ERROR_TOO_MANY_REDIRECTS" : {
    code : tizen.WebAPIException.NETWORK_ERR,
    name : "NetworkError",
    message : "In case of too may redirects from http response header",
  },
  "DOWNLOAD_ERROR_UNHANDLED_HTTP_CODE" : {
    code : tizen.WebAPIException.NETWORK_ERR,
    name : "NetworkError",
    message : "The download cannot handle the HTTP status value",
  },
  "DOWNLOAD_ERROR_SYSTEM_DOWN" : {
    code : tizen.WebAPIException.NOT_SUPPORTED_ERR,
    name : "NotSupportedError",
    message : "There are no response from client after rebooting download daemon",
  },
  "DOWNLOAD_ERROR_NO_DATA" : {
    code : tizen.WebAPIException.NOT_SUPPORTED_ERR,
    name : "NotSupportedError",
    message : "No data because the set API is not called",
  },
  "DOWNLOAD_ERROR_ID_NOT_FOUND" : {
    code : tizen.WebAPIException.NOT_SUPPORTED_ERR,
    name : "NotSupportedError",
    message : "The download id is not existed in download service module",
  },
};

var downloadState = {};

extension.setMessageListener(function(msg) {
  var m = JSON.parse(msg);
  if (m.cmd == "DownloadReplyProgress") {
    downloadState[m.uid] = "DOWNLOADING";
    startListeners[m.uid].onprogress(m.uid, m.receivedSize, m.totalSize);
  } else if (m.cmd == "DownloadReplyComplete") {
    downloadState[m.uid] = "COMPLETED";
    startListeners[m.uid].oncompleted(m.uid, m.fullPath);
  } else if (m.cmd == "DownloadReplyPause") {
    downloadState[m.uid] = "PAUSED";
    startListeners[m.uid].onpaused(m.uid);
  } else if (m.cmd == "DownloadReplyCancel") {
    downloadState[m.uid] = "CANCELED";
    startListeners[m.uid].oncanceled(m.uid);
  } else if (m.cmd == "DownloadReplyFail") {
    downloadState[m.uid] = "FAILED";
    startListeners[m.uid].onfailed(m.uid,
        new tizen.WebAPIError(errorMap[m.errorCode].code,
                              errorMap[m.errorCode].message,
                              errorMap[m.errorCode].name));
  }
});

tizen.DownloadRequest = function(url, destination, filename) {
  this.url = url;
  this.uid = (++currentUID).toString();
  typeof destination != 'undefined' && (this.destination = destination);
  typeof filename != 'undefined' && (this.filename = filename);
}

exports.start = function(request, listener) {
  if (!(request instanceof tizen.DownloadRequest)) {
    console.log(
        "tizen.download.start(): argument of invalid type " + typeof(request));
    return;
  }
  startListeners[request.uid] = listener;
  postMessage({
    "cmd": "DownloadStart",
    "url": request.url,
    "uid": request.uid,
    "destination": request.destination,
    "filename": request.filename,
  });
  return request.uid;
}

exports.setListener = function(downloadId, listener) {
  startListeners[request.uid] = listener;
}

exports.pause = function(downloadId) {
  postMessage({
    "cmd": "DownloadPause",
    "uid": downloadId,
  });
}

exports.resume = function(downloadId) {
  postMessage({
    "cmd": "DownloadResume",
    "uid": downloadId,
  });
}

exports.cancel = function(downloadId) {
  postMessage({
    "cmd": "DownloadCancel",
    "uid": downloadId,
  });
}

// TODO(hdq): Use sync message to get state from manager.
exports.getState = function(downloadId) {
  return downloadState[downloadId];
}

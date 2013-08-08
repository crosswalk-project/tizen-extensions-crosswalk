// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
var postMessage = function (msg) {
    extension.postMessage(JSON.stringify(msg));
};

var errors = [
  { type: "UNKNOWN_ERR", value: 0, name: "UnknownError", message: "" },
  { type: "INDEX_SIZE_ERR", value: 1, name: "IndexSizeError", message: "" },
  { type: "DOMSTRING_SIZE_ERR", value: 2, name: "DOMStringSizeError", message: "" },
  { type: "HIERARCHY_REQUEST_ERR", value: 3, name: "HierarchyRequestError", message: "" },
  { type: "WRONG_DOCUMENT_ERR", value: 4, name: "WrongDocumentError", message: "" },
  { type: "INVALID_CHARACTER_ERR", value: 5, name: "IndexSizeError", message: "" },
  { type: "NO_DATA_ALLOWED_ERR", value: 6, name: "IndexSizeError", message: "" },
  { type: "NO_MODIFICATION_ALLOWED_ERR", value: 7, name: "IndexSizeError", message: "" },
  { type: "NOT_FOUND_ERR", value: 8, name: "IndexSizeError", message: "" },
  { type: "NOT_SUPPORTED_ERR", value: 9, name: "NotSupportedError", message: "" },
  { type: "INUSE_ATTRIBUTE_ERR", value: 10, name: "IndexSizeError", message: "" },
  { type: "INVALID_STATE_ERR", value: 11, name: "IndexSizeError", message: "" },
  { type: "SYNTAX_ERR", value: 12, name: "IndexSizeError", message: "" },
  { type: "INVALID_MODIFICATION_ERR", value: 13, name: "IndexSizeError", message: "" },
  { type: "NAMESPACE_ERR", value: 14, name: "IndexSizeError", message: "" },
  { type: "INVALID_ACCESS_ERR", value: 15, name: "IndexSizeError", message: "" },
  { type: "VALIDATION_ERR", value: 16, name: "IndexSizeError", message: "" },
  { type: "TYPE_MISMATCH_ERR", value: 17, name: "TypeMismatchError", message: "" },
  { type: "SECURITY_ERR", value: 18, name: "IndexSizeError", message: "" },
  { type: "NETWORK_ERR", value: 19, name: "IndexSizeError", message: "" },
  { type: "ABORT_ERR", value: 20, name: "IndexSizeError", message: "" },
  { type: "URL_MISMATCH_ERR", value: 21, name: "IndexSizeError", message: "" },
  { type: "QUOTA_EXCEEDED_ERR", value: 22, name: "IndexSizeError", message: "" },
  { type: "TIMEOUT_ERR", value: 23, name: "IndexSizeError", message: "" },
  { type: "INVALID_NODE_TYPE_ERR", value: 24, name: "IndexSizeError", message: "" },
  { type: "DATA_CLONE_ERR", value: 25, name: "IndexSizeError", message: "" },

  { type: "INVALID_VALUES_ERR", value: 0, name: "IndexSizeError", message: "" },
  { type: "IO_ERR", value: 0, name: "IndexSizeError", message: "" },
  { type: "PERMISSION_DENIED_ERR", value: 0, name: "IndexSizeError", message: "" },
  { type: "SERVICE_NOT_AVAILABLE_ERR", value: 0, name: "IndexSizeError", message: "" },
]

exports.WebAPIException = function (code, message, name) {
  var _code, _message, _name;

  if (typeof code !== 'number') {
    _code = errors[0].value;
    _message = errors[0].message;
    _name = errors[0].name;
  } else {
    _code = code;
    if (typeof message === 'string') {
      _message = message;
    } else {
      _message = errors[_code].message;
    } if (typeof name === 'string') {
      _name = name;
    } else {
      _name = errors[_code].name;
    }
  }

  this.__defineGetter__("code", function () { return _code; });
  this.__defineGetter__("message", function () { return _message; });
  this.__defineGetter__("name", function () { return _name; });
}

for (var i = 0; i < errors.length; i++)
  Object.defineProperty(tizen.WebAPIException, errors[i].type, { value: errors[i].value });

exports.WebAPIError = function (code, message, name) {
  var _code, _message, _name;

  if (typeof code !== 'number') {
    _code = errors[0].value;
    _message = errors[0].message;
    _name = errors[0].name;
  } else {
    _code = code;
    if (typeof message === 'string') {
      _message = message;
    } else {
      _message = errors[_code].message;
    } if (typeof name === 'string') {
      _name = name;
    } else {
      _name = errors[_code].name;
    }
  }

  this.__defineGetter__("code", function () { return _code; });
  this.__defineGetter__("message", function () { return _message; });
  this.__defineGetter__("name", function () { return _name; });
}

// NOTE: Stubs for Application. These are needed for running TCT until
// we have a proper Application API implementation.
exports.application = {
  getAppInfo: function() { return { id: 0 } },
}

exports.ApplicationControlData = function(key, value) {
  this.key = key;
  this.value = value;
}

exports.ApplicationControl = function(operation, uri, mime, category, data) {
  this.operation = operation;
  this.uri = uri;
  this.mime = mime;
  this.category = category;
  this.data = data || [];
}

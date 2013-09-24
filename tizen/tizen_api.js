// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// WARNING! This list should be in sync with the equivalent enum
// located at tizen.h. Remember to update tizen.h if you change
// something here.
var errors = {
  '-1': { type: 'NO_ERROR', name: 'NoError', message: '' },

  '0': { type: 'UNKNOWN_ERR', name: 'UnknownError', message: '' },
  '1': { type: 'INDEX_SIZE_ERR', name: 'IndexSizeError', message: '' },
  '2': { type: 'DOMSTRING_SIZE_ERR', name: 'DOMStringSizeError', message: '' },
  '3': { type: 'HIERARCHY_REQUEST_ERR', name: 'HierarchyRequestError', message: '' },
  '4': { type: 'WRONG_DOCUMENT_ERR', name: 'WrongDocumentError', message: '' },
  '5': { type: 'INVALID_CHARACTER_ERR', name: 'InvalidCharacterError', message: '' },
  '6': { type: 'NO_DATA_ALLOWED_ERR', name: 'NoDataAllowedError', message: '' },
  '7': { type: 'NO_MODIFICATION_ALLOWED_ERR', name: 'NoModificationAllowedError', message: '' },
  '8': { type: 'NOT_FOUND_ERR', name: 'NotFoundError', message: '' },
  '9': { type: 'NOT_SUPPORTED_ERR', name: 'Not_supportedError', message: '' },
  '10': { type: 'INUSE_ATTRIBUTE_ERR', name: 'InuseAttributeError', message: '' },
  '11': { type: 'INVALID_STATE_ERR', name: 'InvalidStateError', message: '' },
  '12': { type: 'SYNTAX_ERR', name: 'SyntaxError', message: '' },
  '13': { type: 'INVALID_MODIFICATION_ERR', name: 'InvalidModificationError', message: '' },
  '14': { type: 'NAMESPACE_ERR', name: 'NamespaceError', message: '' },
  '15': { type: 'INVALID_ACCESS_ERR', name: 'InvalidAccessError', message: '' },
  '16': { type: 'VALIDATION_ERR', name: 'ValidationError', message: '' },
  '17': { type: 'TYPE_MISMATCH_ERR', name: 'TypeMismatchError', message: '' },
  '18': { type: 'SECURITY_ERR', name: 'SecurityError', message: '' },
  '19': { type: 'NETWORK_ERR', name: 'NetworkError', message: '' },
  '20': { type: 'ABORT_ERR', name: 'AbortError', message: '' },
  '21': { type: 'URL_MISMATCH_ERR', name: 'UrlMismatchError', message: '' },
  '22': { type: 'QUOTA_EXCEEDED_ERR', name: 'QuotaExceededError', message: '' },
  '23': { type: 'TIMEOUT_ERR', name: 'TimeoutError', message: '' },
  '24': { type: 'INVALID_NODE_TYPE_ERR', name: 'InvalidNodeTypeError', message: '' },
  '25': { type: 'DATA_CLONE_ERR', name: 'DataCloneError', message: '' },

  // Error codes for these errors are not really defined anywhere.
  '100': { type: 'INVALID_VALUES_ERR', name: 'InvalidValuesError', message: '' },
  '101': { type: 'IO_ERR', name: 'IoError', message: '' },
  '102': { type: 'PERMISSION_DENIED_ERR', name: 'Permission_deniedError', message: '' },
  '103': { type: 'SERVICE_NOT_AVAILABLE_ERR', name: 'ServiceNotAvailableError', message: '' }
};

exports.WebAPIException = function(code, message, name) {
  var _code, _message, _name;

  if (typeof code !== 'number') {
    _code = 0;
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

  this.__defineGetter__('code', function() { return _code; });
  this.__defineGetter__('message', function() { return _message; });
  this.__defineGetter__('name', function() { return _name; });
};

for (var value in errors)
  Object.defineProperty(tizen.WebAPIException, errors[value].type, { value: parseInt(value) });

exports.WebAPIError = function(code, message, name) {
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

  this.__defineGetter__('code', function() { return _code; });
  this.__defineGetter__('message', function() { return _message; });
  this.__defineGetter__('name', function() { return _name; });
};

for (var value in errors)
  Object.defineProperty(tizen.WebAPIError, errors[value].type, { value: value });

// NOTE: Stubs for Application. These are needed for running TCT until
// we have a proper Application API implementation.
exports.application = {
  getAppInfo: function() { return { id: 0 } }
};

exports.ApplicationControlData = function(key, value) {
  this.key = key;
  this.value = value;
};

exports.ApplicationControl = function(operation, uri, mime, category, data) {
  this.operation = operation;
  this.uri = uri;
  this.mime = mime;
  this.category = category;
  this.data = data || [];
};

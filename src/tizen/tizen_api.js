// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Tizen API Specification:
// https://developer.tizen.org/dev-guide/2.2.1/org.tizen.web.device.apireference/tizen/tizen.html


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
  '101': { type: 'IO_ERR', name: 'IOError', message: 'IOError' },
  '102': { type: 'PERMISSION_DENIED_ERR', name: 'Permission_deniedError', message: '' },
  '103': { type: 'SERVICE_NOT_AVAILABLE_ERR', name: 'ServiceNotAvailableError', message: '' },
  '104': { type: 'DATABASE_ERR', name: 'DATABASE_ERR', message: '' }
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
  Object.defineProperty(exports.WebAPIException, errors[value].type, { value: parseInt(value) });

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
  Object.defineProperty(exports.WebAPIError, errors[value].type, { value: value });

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

// Tizen Filters

// either AttributeFilter, AttributeRangeFilter, or CompositeFilter
function is_tizen_filter(f) {
  return (f instanceof tizen.AttributeFilter) ||
         (f instanceof tizen.AttributeRangeFilter) ||
         (f instanceof tizen.CompositeFilter);
}

// AbstractFilter (abstract base class)
exports.AbstractFilter = function() {};

// SortMode
// [Constructor(DOMString attributeName, optional SortModeOrder? order)]
// interface SortMode {
//   attribute DOMString attributeName;
//   attribute SortModeOrder order;
// };
exports.SortMode = function(attrName, order) {
  if (!(typeof(attrName) === 'string' || attrName instanceof String) ||
      order && (order != 'DESC' && order != 'ASC'))
    throw new exports.WebAPIException(exports.WebAPIException.TYPE_MISMATCH_ERR);

  var attributeName_ = attrName;
  var order_ = order != null ? order : 'ASC';
  Object.defineProperty(this, 'attributeName', {
    enumerable: true,
    get: function() { return attributeName_; },
    set: function(value) { if (value != null) attributeName_ = value; }
  });
  Object.defineProperty(this, 'order', {
    enumerable: true,
    get: function() { return order_; },
    set: function(value) { if (value == 'ASC' || value == 'DESC') order_ = value; }
  });
};
exports.SortMode.prototype.constructor = exports.SortMode;

// AttributeFilter
// [Constructor(DOMString attributeName, optional FilterMatchFlag? matchFlag,
//              optional any matchValue)]
// interface AttributeFilter : AbstractFilter {
//   attribute DOMString attributeName;
//   attribute FilterMatchFlag matchFlag;
//   attribute any matchValue;
// };

var FilterMatchFlag = {
  EXACTLY: 0,
  FULLSTRING: 1,
  CONTAINS: 2,
  STARTSWITH: 3,
  ENDSWITH: 4,
  EXISTS: 5
};

exports.AttributeFilter = function(attrName, matchFlag, matchValue) {
  if (this && this.constructor == exports.AttributeFilter &&
      (typeof(attrName) === 'string' || attrName instanceof String) &&
      (matchFlag == null || matchFlag in FilterMatchFlag)) {
    var attributeName_ = attrName;
    var matchFlag_ = matchFlag == null ? 'EXACTLY' : matchFlag;
    var matchValue_ = matchValue == null ? null : matchValue;

    Object.defineProperty(this, 'attributeName', {
      enumerable: true,
      get: function() { return attributeName_; },
      set: function(value) { if (value != null) attributeName_ = value; }
    });
    Object.defineProperty(this, 'matchFlag', {
      enumerable: true,
      get: function() { return matchFlag_; },
      set: function(value) { if (value in FilterMatchFlag) matchFlag_ = value; }
    });
    Object.defineProperty(this, 'matchValue', {
      enumerable: true,
      get: function() { return matchValue_; },
      set: function(value) { value === undefined ? matchValue_ = null : matchValue_ = value }
    });
  } else {
    throw new exports.WebAPIException(exports.WebAPIException.TYPE_MISMATCH_ERR);
  }
};
exports.AttributeFilter.prototype = new exports.AbstractFilter();
exports.AttributeFilter.prototype.constructor = exports.AttributeFilter;


// AttributeRangeFilter
// [Constructor(DOMString attributeName, optional any initialValue,
//              optional any endValue)]
// interface AttributeRangeFilter : AbstractFilter {
//   attribute DOMString attributeName;
//   attribute any initialValue;
//   attribute any endValue;
// };
exports.AttributeRangeFilter = function(attrName, start, end) {
  if (!this || this.constructor != exports.AttributeRangeFilter ||
      !(typeof(attrName) === 'string' || attrname instanceof String)) {
    throw new exports.WebAPIException(exports.WebAPIException.TYPE_MISMATCH_ERR);
  }

  var attributeName_ = attrName;
  Object.defineProperty(this, 'attributeName', {
    enumerable: true,
    get: function() { return attributeName_; },
    set: function(value) { if (value != null) attributeName_ = value; }
  });
  Object.defineProperty(this, 'initialValue', {
    writable: true,
    enumerable: true,
    value: start === undefined ? null : start
  });
  Object.defineProperty(this, 'endValue', {
    writable: true,
    enumerable: true,
    value: end === undefined ? null : end
  });
};
exports.AttributeRangeFilter.prototype = new exports.AbstractFilter();
exports.AttributeRangeFilter.prototype.constructor = exports.AttributeRangeFilter;


// CompositeFilter
// [Constructor(CompositeFilterType type, optional AbstractFilter[]? filters)]
// interface CompositeFilter : AbstractFilter {
//   attribute CompositeFilterType type;
//   attribute AbstractFilter[] filters;
// };

var CompositeFilterType = { UNION: 0, INTERSECTION: 1 };

exports.CompositeFilter = function(type, filters) {
  if (!this || this.constructor != exports.CompositeFilter ||
      !(type in CompositeFilterType) ||
      filters && !(filters instanceof Array)) {
    throw new exports.WebAPIException(exports.WebAPIException.TYPE_MISMATCH_ERR);
  }

  var type_ = type;
  var filters_ = filters;
  Object.defineProperty(this, 'type', {
    enumerable: true,
    get: function() { return type_; },
    set: function(value) { if (value in CompositeFilterType) type_ = value; }
  });
  Object.defineProperty(this, 'filters', {
    enumerable: true,
    get: function() { return filters_; },
    set: function(value) { if (value != null) filters_ = value; }
  });
};
exports.CompositeFilter.prototype = new exports.AbstractFilter();
exports.CompositeFilter.prototype.constructor = exports.CompositeFilter;

// end of Tizen filters

// SimpleCoordinates
// [Constructor(double latitude, double longitude)]
// interface SimpleCoordinates {
//   attribute double latitude;
//   attribute double longitude;
// };
exports.SimpleCoordinates = function(latitude, longitude) {
  if (!(typeof(latitude) === 'number' || typeof(longitude) === 'number'))
    throw new exports.WebAPIException(exports.WebAPIException.TYPE_MISMATCH_ERR);

  var latitude_ = latitude;
  var longitude_ = longitude;
  Object.defineProperty(this, 'latitude', {
    enumerable: true,
    get: function() { return latitude_; },
    set: function(value) {
      if (value != null && typeof(latitude) === 'number') latitude_ = value;
    }
  });
  Object.defineProperty(this, 'longitude', {
    enumerable: true,
    get: function() { return longitude_; },
    set: function(value) {
      if (value != null && typeof(longitude) === 'number') longitude_ = value;
    }
  });

};
exports.SimpleCoordinates.prototype.constructor = exports.SimpleCoordinates;

// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// FIXME: A lot of these methods should throw NOT_SUPPORTED_ERR on desktop.
// There is no easy way to do verify which methods are supported yet.

var brightness = undefined;
var screenState = undefined;

var listeners = [];
function callListeners(oldState, newState) {
  listeners.forEach(function(listener) {
    listener(previousState, changedState);
  });
}

var postMessage = function(msg) {
  extension.postMessage(JSON.stringify(msg));
};

extension.setMessageListener(function(msg) {
  var m = JSON.parse(msg);
  if (m.cmd == "ScreenStateChanged") {
    brightness = m.brightness;
    oldScreenState = screenState;
    screenState = m.state;
    if (oldScreenState !== screenState)
      callListeners(oldScreenState, screenState);
  }
});

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
  { type: "NOT_SUPPORTED_ERR", value: 9, name: "IndexSizeError", message: "" },
  { type: "INUSE_ATTRIBUTE_ERR", value: 10, name: "IndexSizeError", message: "" },
  { type: "INVALID_STATE_ERR", value: 11, name: "IndexSizeError", message: "" },
  { type: "SYNTAX_ERR", value: 12, name: "IndexSizeError", message: "" },
  { type: "INVALID_MODIFICATION_ERR", value: 13, name: "IndexSizeError", message: "" },
  { type: "NAMESPACE_ERR", value: 14, name: "IndexSizeError", message: "" },
  { type: "INVALID_ACCESS_ERR", value: 15, name: "IndexSizeError", message: "" },
  { type: "VALIDATION_ERR", value: 16, name: "IndexSizeError", message: "" },
  { type: "TYPE_MISMATCH_ERR", value: 17, name: "IndexSizeError", message: "" },
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

tizen.WebAPIException = function(code, message, name) {
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

var resources = {
  "SCREEN": {
    type: 0,
    states: {
      "SCREEN_OFF": 0,
      "SCREEN_DIM": 1,
      "SCREEN_NORMAL": 2,
      "SCREEN_BRIGHT": 3 // Deprecated.
    }
  },
  "CPU": {
    type: 1,
    states: {
      "CPU_AWAKE": 4
    }
  }
}

exports.request = function(resource, state) {
  // Validate permission to 'power'.
  // throw new WebAPIException(SECURITY_ERR);

  if (typeof resource !== 'string' || typeof state !== 'string') {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  if (!resources.hasOwnProperty(resource)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);
  }

  if (!resources[resource].states.hasOwnProperty(state)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);
  }

  // Exception check: SCREEN_OFF is a state cannot be requested
  if (resource === "SCREEN" && state === "SCREEN_OFF") {
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);
  }

  postMessage({
    "cmd": "PowerRequest",
    "resource": resources[resource].type,
    "state": resources[resource].states[state]
  });
}

exports.release = function(resource) {
  // Validate permission to 'power'. Do not throw, just bail out.

  if (typeof resource !== 'string')
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  if (!resources.hasOwnProperty(resource))
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);

   postMessage({
    "cmd": "PowerRelease",
    "resource": resources[resource].type
  });
}

exports.setScreenStateChangeListener = function(listener) {
  // No permission validation.

  if (typeof listener !== 'function')
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  // FIXME: According to docs, it should throw INVALID_VALUES_ERR if input
  // parameters contain an invalid value. Verify the Tizen 2.x impl.

  listeners.push(listener);
}

exports.unsetScreenStateChangeListener = function() {
  // No permission validation.
  listeners = [];
}

exports.getScreenBrightness = function() {
  if (typeof brightness !== 'number')
    throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);
  return brightness;
}

exports.setScreenBrightness = function(brightness) {
  // Validate permission to 'power'.
  // throw new WebAPIException(SECURITY_ERR);

  if (typeof brightness !== 'number')
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  if (brightness < 0 || brightness > 1)
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);

  postMessage({
    "cmd": "PowerSetScreenBrightness",
    "value": brightness
  });
}

exports.isScreenOn = function() {
   if (typeof screenState !== 'number')
    throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);
  return screenState !== resources["SCREEN"].states["SCREEN_OFF"]
}

exports.restoreScreenBrightness = function() {
  // Validate permission to 'power'.
  // throw new WebAPIException(SECURITY_ERR);

  // FIXME: throw UNKNOWN_ERR during failure to set the new value.

  postMessage({
    "cmd": "PowerSetScreenBrightness",
    "value": -1
  });
}

exports.turnScreenOff = function() {
  // Validate permission to 'power'.
  // throw new WebAPIException(SECURITY_ERR);

  // FIXME: throw UNKNOWN_ERR during failure to set the new value.
  postMessage({
    "cmd": "PowerSetScreenEnabled",
    "value": true
  });
}

exports.turnScreenOn = function() {
  // Validate permission to 'power'.
  // throw new WebAPIException(SECURITY_ERR);

  // FIXME: throw UNKNOWN_ERR during failure to set the new value.
  postMessage({
    "cmd": "PowerSetScreenEnabled",
    "value": true
  });
}

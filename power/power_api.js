// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// FIXME: A lot of these methods should throw NOT_SUPPORTED_ERR on desktop.
// There is no easy way to do verify which methods are supported yet.

var screenState = undefined;
var defaultScreenBrightness = undefined;

var resources = {
  'SCREEN': {
    type: 0,
    states: {
      'SCREEN_OFF': 0,
      'SCREEN_DIM': 1,
      'SCREEN_NORMAL': 2,
      'SCREEN_BRIGHT': 3 // Deprecated.
    }
  },
  'CPU': {
    type: 1,
    states: {
      'CPU_AWAKE': 4
    }
  }
};

var listeners = [];
function callListeners(oldState, newState) {
  var previousState = Object.keys(resources['SCREEN'].states)[oldState];
  var changedState = Object.keys(resources['SCREEN'].states)[newState];
  listeners.forEach(function(listener) {
    listener(previousState, changedState);
  });
}

var postMessage = function(msg) {
  extension.postMessage(JSON.stringify(msg));
};

var sendSyncMessage = function(msg) {
  return extension.internal.sendSyncMessage(JSON.stringify(msg));
};

function getScreenState() {
    var msg = {
      'cmd': 'PowerGetScreenState'
    };
    var r = JSON.parse(sendSyncMessage(msg));
    screenState = r.state;
}

extension.setMessageListener(function(msg) {
  var m = JSON.parse(msg);
  if (m.cmd == 'ScreenStateChanged') {
    var newState = m.state;
    if (screenState == undefined)
      getScreenState();
    if (screenState !== newState) {
      callListeners(screenState, newState);
      screenState = newState;
    }
  }
});

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
  if (resource === 'SCREEN' && state === 'SCREEN_OFF') {
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);
  }

  postMessage({
    'cmd': 'PowerRequest',
    'resource': resources[resource].type,
    'state': resources[resource].states[state]
  });
};

exports.release = function(resource) {
  // Validate permission to 'power'. Do not throw, just bail out.

  if (typeof resource !== 'string')
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  if (!resources.hasOwnProperty(resource))
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);

  postMessage({
    'cmd': 'PowerRelease',
    'resource': resources[resource].type
  });
};

exports.setScreenStateChangeListener = function(listener) {
  // No permission validation.

  if (typeof listener !== 'function')
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  // FIXME: According to docs, it should throw INVALID_VALUES_ERR if input
  // parameters contain an invalid value. Verify the Tizen 2.x impl.

  listeners.push(listener);
};

exports.unsetScreenStateChangeListener = function() {
  // No permission validation.
  listeners = [];
};

exports.getScreenBrightness = function() {
  var brightness = parseFloat(sendSyncMessage({
    'cmd': 'PowerGetScreenBrightness'
  }));
  return brightness;
};

defaultScreenBrightness = exports.getScreenBrightness();

exports.setScreenBrightness = function(brightness) {
  // Validate permission to 'power'.
  // throw new WebAPIException(SECURITY_ERR);
  if (typeof brightness !== 'number' || !isFinite(brightness))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  if (brightness < 0 || brightness > 1)
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);

  postMessage({
    'cmd': 'PowerSetScreenBrightness',
    'value': brightness
  });
};

exports.isScreenOn = function() {
  getScreenState();
  if (typeof screenState !== 'number')
    throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);
  return screenState !== resources['SCREEN'].states['SCREEN_OFF'];
};

exports.restoreScreenBrightness = function() {
  // Validate permission to 'power'.
  // throw new WebAPIException(SECURITY_ERR);

  if (defaultScreenBrightness < 0 || defaultScreenBrightness > 1)
    throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);

  postMessage({
    'cmd': 'PowerSetScreenBrightness',
    'value': defaultScreenBrightness
  });
};

exports.turnScreenOff = function() {
  // Validate permission to 'power'.
  // throw new WebAPIException(SECURITY_ERR);

  // FIXME: throw UNKNOWN_ERR during failure to set the new value.
  postMessage({
    'cmd': 'PowerSetScreenEnabled',
    'value': false
  });
};

exports.turnScreenOn = function() {
  // Validate permission to 'power'.
  // throw new WebAPIException(SECURITY_ERR);

  // FIXME: throw UNKNOWN_ERR during failure to set the new value.
  postMessage({
    'cmd': 'PowerSetScreenEnabled',
    'value': true
  });
};

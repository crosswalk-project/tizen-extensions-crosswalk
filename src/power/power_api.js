// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// FIXME: A lot of these methods should throw NOT_SUPPORTED_ERR on desktop.
// There is no easy way to do verify which methods are supported yet.

// Save the latest screenState value. We use this to pass the previous
// state when calling the listener.
var screenState = undefined;

var defaultScreenBrightness = undefined;
var screenStateChangedListener;

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

function callListener(oldState, newState) {
  if (screenStateChangedListener == null)
    return;
  var previousState = resources.SCREEN.states[oldState];
  var changedState = resources.SCREEN.states[newState];
  screenStateChangedListener(oldState, newState);
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
    if (screenState !== newState) {
      callListener(screenState, newState);
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

  // This will cache an initial value, that is necessary to ensure we
  // always have a previous value.
  getScreenState();

  screenStateChangedListener = listener;
  postMessage({
    'cmd': 'SetListenToScreenStateChange',
    'value': true
  });
};

exports.unsetScreenStateChangeListener = function() {
  // No permission validation.
  screenStateChangedListener = null;
  postMessage({
    'cmd': 'SetListenToScreenStateChange',
    'value': false
  });
};

exports.getScreenBrightness = function() {
  var r = JSON.parse(sendSyncMessage({
    'cmd': 'PowerGetScreenBrightness'
  }));
  if (r['error']) {
    throw new tizen.WebAPIException(tizen.WebAPIException.NOT_SUPPORTED_ERR);
    return;
  }
  var brightness = r['brightness'];
  if (defaultScreenBrightness === undefined)
    defaultScreenBrightness = brightness;
  return brightness;
};

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

  if (defaultScreenBrightness === undefined)
    exports.getScreenBrightness();

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

// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The W3C Telephony API specification:
// http://www.w3.org/2012/sysapps/telephony/

// The native part of the extension may or may not cache call objects on their
// own, depending on the backend. The oFono backend does cache the calls which
// are handled by oFono as DBUS objects. Because the IPC overhead, call and
// service objects are also cached by this implementation, and the IPC protocol
// involves exchanging object id's - except when there is a mismatch, in which
// case a re-sync is done.

///////////////////////////////////////////////////////////////////////////////
// Utilities
///////////////////////////////////////////////////////////////////////////////

var v8tools = null;
if (typeof requireNative !== 'undefined') {
  v8tools = requireNative('v8tools');
}

if (!v8tools) {
  v8tools = function() {};
  v8tools.prototype.forceSetProperty = function(obj, prop, value) {
    obj[prop] = value;
  };
}

function addReadonlyProperty(obj, name, value) {
  Object.defineProperty(obj, name, {
    configurable: true,
    enumerable: true,
    writable: true,  // TODO zolkis: fix v8tools.forceSetProperty issues
    value: value
  });
}

function forceSetProperty(obj, prop, value) {
  v8tools.forceSetProperty(obj, prop, value);
  //obj[prop] = value;
}

function error(txt) {
  var output = (txt instanceof Object) ?
               '\n[Telephony JS] ' + toPrintableString(txt) :
               '\n[Telephony JS] Error: ' + (txt || 'null');

  console.log(output);
}

function log(txt) {
  console.log('\n[JS log] ' + txt instanceof Object ?
      toPrintableString(txt) : (txt || 'null'));
}

function toPrintableString(o) {
  if (!(o instanceof Object) && !(o instanceof Array))
    return o;
  var out = '{ ';
  for (var i in o) {
    out += i + ': ' + toPrintableString(o[i]) + ', ';
  }
  out += '}';
  return out;
}

function checkRemoteParty(str) {
  return str.match(/[+]?[0-9*#]{1,80}/g) == str;
}

///////////////////////////////////////////////////////////////////////////////
// exports
///////////////////////////////////////////////////////////////////////////////

function TelephonyManager() {
  this.onserviceadded = null;
  this.onserviceremoved = null;
  this.ondefaultservicechanged = null;
  this.oncalladded = null;
  this.callchanged = null;
  this.oncallremoved = null;
  this.onactivecallchanged = null;
  this.oncallstatechanged = null;
  this.onemergencynumberschanged = null;
  addReadonlyProperty(this, 'defaultServiceId', null);
  addReadonlyProperty(this, 'activeCall', null);
}

var _telephonyManager = new TelephonyManager();
exports = _telephonyManager;

///////////////////////////////////////////////////////////////////////////////
// Local structures, event listeners
///////////////////////////////////////////////////////////////////////////////

var _next_promise_id = 1;
var _pending_promises = {};

var _listeners_count = 0;
var _listeners = {};
_listeners.serviceadded = [];
_listeners.serviceremoved = [];
_listeners.defaultservicechanged = [];
_listeners.calladded = [];
_listeners.callremoved = [];
_listeners.callstatechanged = [];
_listeners.callchanged = [];
_listeners.activecallchanged = [];

function addEventListener(kind, callback) {
  if (typeof callback == 'function' && _listeners[kind] &&
      _listeners[kind].indexOf(callback) == -1) {
    _listeners[kind].push(callback);
    if (_listeners_count++ == 0)
      enableBackendNotifications();
  }
}

function removeEventListener(kind, callback) {
  if (typeof callback == 'function' && _listeners[kind]) {
    var i = _listeners[kind].indexOf(callback);
    if (i >= 0) {
      _listeners[kind].splice(i, 1);
      if (--_listeners_count == 0)
        disableBackendNotifications();
    }
  }
}

function dispatchEvent(event) {
  if (typeof event == 'object' && _listeners[event.type]) {
    _listeners[event.type].forEach(function(cb) {
      if (typeof cb == 'function') {
        cb(event);
      }
    });
    return true;
  }
  return false;
}

TelephonyManager.prototype.addEventListener = addEventListener;
TelephonyManager.prototype.removeEventListener = removeEventListener;
TelephonyManager.prototype.dispatchEvent = dispatchEvent;

///////////////////////////////////////////////////////////////////////////////
// Message sending and Promises
///////////////////////////////////////////////////////////////////////////////

function PendingRequest(resolve, reject) {
  this.resolve = resolve;
  this.reject = reject;
}

function sendRequest(msg, resolve, reject) {
  msg.promiseId = _next_promise_id++;
  _pending_promises[msg.promiseId] = new PendingRequest(resolve, reject);
  var req = JSON.stringify(msg);
  extension.postMessage(req);
}

function resolvePromise(msg) {
  var promise = msg.promiseId ? _pending_promises[msg.promiseId] : null;
  if (promise && promise.resolve)
    promise.resolve(msg.returnValue);
}

function rejectPromise(msg) {
  var promise = msg.promiseId ? _pending_promises[msg.promiseId] : null;
  if (!promise || !promise.reject)
    return;
  // DOMError will be nuked, DOMException kludgy to create, so using Error
  // with the properties of DOMException
  // the W3C Telephony spec needs an update on this...
  var e = new Error(msg.errorMessage);
  e.code = msg.errorCode;  // add a 'code' property
  // optionally add a name property; e.name = msg.errorName;
  promise.reject(e);
}

// Handle replies and notifications from native extension code.
extension.setMessageListener(function(json) {
  var msg = JSON.parse(json);
  switch (msg.cmd) {
    // Event notifications.
    case 'activeCallChanged':
      handleActiveCallChanged(msg);
      break;
    case 'callAdded':  // incoming or waiting calls
      handleCallAdded(msg);
      break;
    case 'callRemoved':  // disconnected calls
      handleCallRemoved(msg);
      break;
    case 'callStateChanged':
      handleCallStateChanged(msg);
      break;
    case 'callChanged':
      handleCallChanged(msg);
      break;
    case 'serviceAdded':
      handleServiceAdded(msg);
      break;
    case 'serviceRemoved':
      handleServiceRemoved(msg);
      break;
    case 'serviceChanged':
      handleServiceChanged(msg);
      break;
    case 'defaultServiceChanged':
      handleDefaultServiceChanged(msg);
      break;
    case 'emergencyNumbersChanged':
      handleEmergencyNumbersChanged(msg);
      break;

    // Replies needing postprocessing before resolving the Promise.
    case 'getServices':
      getServicesCallback(msg);
      break;
    case 'getCalls':
      getCallsCallback(msg);
      break;
    case 'getParticipants':
      getParticipantsCallback(msg);
      break;

    // Replies needing no special treatment before resolving the Promise.
    case 'setDefaultService':
    case 'setServiceEnabled':
    case 'dial':
    case 'accept':
    case 'disconnect':
    case 'hold':
    case 'resume':
    case 'deflect':
    case 'transfer':
    case 'createConference':
    case 'split':
    case 'getEmergencyNumbers':
    case 'emergencyDial':
    case 'sendTones':
    case 'startTone':
    case 'stopTone':
      msg.isError ? rejectPromise(msg) : resolvePromise(msg);
      break;
    default:
      error('Invalid message from extension: ' + json);
  }
});

function enableBackendNotifications() {
  var msg = {
    'cmd': 'enableNotifications'
  };
  return sendRequest(msg, null, null);  // no Promise
}

function disableBackendNotifications() {
  var msg = {
    'cmd': 'disableNotifications'
  };
  return sendRequest(msg, null, null);  // no Promise
}

///////////////////////////////////////////////////////////////////////////////
// TelephonyCall
///////////////////////////////////////////////////////////////////////////////

function TelephonyCall(dict) {
  addReadonlyProperty(this, 'callId', dict.callId || null);
  addReadonlyProperty(this, 'conferenceId', dict.conferenceId || null);
  addReadonlyProperty(this, 'remoteParty', dict.remoteParty || null);
  addReadonlyProperty(this, 'serviceId', dict.serviceId || null);
  addReadonlyProperty(this, 'state', dict.state || null);
  addReadonlyProperty(this, 'stateReason', null);
}

TelephonyCall.prototype.accept = function() {
  var msg = {
    'cmd': 'accept',
    'callId': this.callId
  };
  return new Promise(function(resolve, reject) {
    sendRequest(msg, resolve, reject);
  });
};

TelephonyCall.prototype.disconnect = function() {
  var msg = {
    'cmd': 'disconnect',
    'callId': this.callId
  };
  return new Promise(function(resolve, reject) {
    sendRequest(msg, resolve, reject);
  });
};

TelephonyCall.prototype.hold = function() {
  var msg = {
    'cmd': 'hold',
    'callId': this.callId
  };
  return new Promise(function(resolve, reject) {
    sendRequest(msg, resolve, reject);
  });
};

TelephonyCall.prototype.resume = function() {
  var msg = {
    'cmd': 'resume',
    'callId': this.callId
  };
  return new Promise(function(resolve, reject) {
    sendRequest(msg, resolve, reject);
  });
};

TelephonyCall.prototype.deflect = function(remoteParty) {
  var msg = {
    'cmd': 'deflect',
    'callId': this.callId,
    'remoteParty': remoteParty
  };
  return new Promise(function(resolve, reject) {
    sendRequest(msg, resolve, reject);
  });
};

TelephonyCall.prototype.transfer = function(call) {
  var msg = {
    'cmd': 'transfer'
  };
  return new Promise(function(resolve, reject) {
    sendRequest(msg, resolve, reject);
  });
};

TelephonyCall.prototype.split = function() {
  var msg = {
    'cmd': 'split',
    'callId': this.callId
  };
  return new Promise(function(resolve, reject) {
    sendRequest(msg, resolve, reject);
  });
};

///////////////////////////////////////////////////////////////////////////////
// Call Management
///////////////////////////////////////////////////////////////////////////////

function handleActiveCallChanged(msg) {
  var call = msg.call ? new TelephonyCall(msg.call) : null;
  forceSetProperty(_telephonyManager, 'activeCall', call);
  var evt = new Event('activecallchanged');
  _telephonyManager.dispatchEvent(evt);
  if (_telephonyManager.onactivecallchanged instanceof Function)
    _telephonyManager.onactivecallchanged(evt);
}

function handleCallAdded(msg) {
  var evt = new CustomEvent('calladded');
  addReadonlyProperty(evt, 'call', new TelephonyCall(msg.call));
  _telephonyManager.dispatchEvent(evt);
  if (_telephonyManager.oncalladded instanceof Function)
    _telephonyManager.oncalladded(evt);
}

function handleCallStateChanged(msg) {
  var call = new TelephonyCall(msg.call);
  if (call.state == 'disconnected' && !call.duration) {
    // if the backend didn't calculate duration, do it now
    call.duration = new Date() - new Date(call.startTime);  // milliseconds
  }
  if (call.callId == _telephonyManager.activeCall.callId)
    forceSetProperty(_telephonyManager, 'activeCall', call);
  var evt = new CustomEvent('callstatechanged');
  addReadonlyProperty(evt, 'call', call);
  _telephonyManager.dispatchEvent(evt);
  if (_telephonyManager.oncallstatechanged instanceof Function)
    _telephonyManager.oncallstatechanged(evt);
}

// special things to handle: isConference is changed with conf calls
function handleCallChanged(msg) {
  if (!msg.call || !msg.changedProperties)
    return;
  var call = new TelephonyCall(msg.call);
  if (call.callId == _telephonyManager.activeCall.callId)
    forceSetProperty(_telephonyManager, 'activeCall', call);
  var evt = new CustomEvent('callchanged');
  addReadonlyProperty(evt, 'call', call);
  addReadonlyProperty(evt, 'changedProperties', call.changedProperties || []);
  _telephonyManager.dispatchEvent(evt);
  if (_telephonyManager.oncallchanged instanceof Function)
    _telephonyManager.oncallchanged(evt);
}

function handleCallRemoved(msg) {
  var evt = new CustomEvent('callremoved');
  addReadonlyProperty(evt, 'call', new TelephonyCall(msg.call));
  _telephonyManager.dispatchEvent(evt);
  if (_telephonyManager.oncallremoved instanceof Function)
    _telephonyManager.oncallremoved(evt);
}

TelephonyManager.prototype.getCalls = function() {
  var msg = {
    'cmd': 'getCalls'
  };
  return new Promise(function(resolve, reject) {
    sendRequest(msg, resolve, reject);
  });
};

function getCallsCallback(msg) {
  if (msg.isError || !(msg.returnValue instanceof Array)) {
    rejectPromise(msg);
    return;
  }
  var res = [];
  for (var i = 0; i < msg.returnValue.length; i++) {
    res.push(new TelephonyCall(msg.returnValue[i]));
  }
  msg.returnValue = res;
  resolvePromise(msg);
}

TelephonyManager.prototype.dial = function(remoteParty, dialOptions) {
  return new Promise(function(resolve, reject) {
    if (!checkRemoteParty(remoteParty)) {
      var err = new Error('InvalidCharacterError');
      err.message = 'Invalid remote party';
      reject(err);
      return;
    }
    var msg = {
      cmd: 'dial',
      serviceId: dialOptions && dialOptions.serviceId || null,
      remoteParty: remoteParty,
      hideCallerId: dialOptions && dialOptions.hideCallerId || false
    };
    sendRequest(msg, resolve, reject);
  });
};

TelephonyManager.prototype.createConference = function() {
  var msg = {
    'cmd': 'createConference'
  };
  return new Promise(function(resolve, reject) {
    sendRequest(msg, resolve, reject);
  });
};

TelephonyManager.prototype.getParticipants = function(conferenceId) {
  var msg = {
    'cmd': 'getParticipants',
    'conferenceId': conferenceId
  };
  return new Promise(function(resolve, reject) {
    sendRequest(msg, resolve, reject);
  });
};

function getParticipantsCallback(msg) {
  if (msg.isError || !(msg.returnValue instanceof Array)) {
    rejectPromise(msg);
    return;
  }
  var res = [];
  for (var i = 0; i < msg.returnValue.length; i++)
    res.push(new TelephonyCall(msg.returnValue[i]));
  msg.returnValue = res;
  resolvePromise(msg);
}

///////////////////////////////////////////////////////////////////////////////
// Tone Management
///////////////////////////////////////////////////////////////////////////////

TelephonyManager.prototype.sendTones = function(tones, toneOptions) {
  var msg = {
    'cmd': 'sendTones',
    'tones': tones,
    'serviceId': toneOptions && toneOptions.serviceId || null,
    'gap': toneOptions && toneOptions.gap || null,
    'duration': toneOptions && toneOptions.duration || null
  };
  return new Promise(function(resolve, reject) {
    sendRequest(msg, resolve, reject);
  });
};

TelephonyManager.prototype.startTone = function(tone, toneOptions) {
  var msg = {
    'cmd': 'startTone',
    'tone': tone,
    'serviceId': toneOptions && toneOptions.serviceId || null,
    'gap': toneOptions && toneOptions.gap || ''
  };
  return new Promise(function(resolve, reject) {
    sendRequest(msg, resolve, reject);
  });
};

TelephonyManager.prototype.stopTone = function(serviceId, toneOptions) {
  var msg = {
    'cmd': 'stopTone',
    'serviceId': toneOptions && toneOptions.serviceId || null
  };
  return new Promise(function(resolve, reject) {
    sendRequest(msg, resolve, reject);
  });
};

///////////////////////////////////////////////////////////////////////////////
// Emergency Management
///////////////////////////////////////////////////////////////////////////////

TelephonyManager.prototype.getEmergencyNumbers = function() {
  var msg = {
    'cmd': 'getEmergencyNumbers'
  };
  return new Promise(function(resolve, reject) {
    sendRequest(msg, resolve, reject);
  });
};

///////////////////////////////////////////////////////////////////////////////
// TelephonyService
///////////////////////////////////////////////////////////////////////////////

function TelephonyService(dict) {
  if (!dict)
    return null;
  addReadonlyProperty(this, 'serviceId', dict.id || null);
  addReadonlyProperty(this, 'enabled', !!dict.enabled);
  addReadonlyProperty(this, 'emergency', dict.emergency);
  addReadonlyProperty(this, 'protocol', dict.protocol || '');
  addReadonlyProperty(this, 'serviceType', dict.type || '');
  addReadonlyProperty(this, 'provider', dict.provider || '');
  this.displayName = dict.name || '';
  return this;
}

TelephonyService.prototype.setServiceEnabled = function(enabled) {
  var msg = {
    'cmd': 'setServiceEnabled',
    'serviceId': this.serviceId,
    'enabled': enabled ? true : false
  };
  return new Promise(function(resolve, reject) {
    sendRequest(msg, resolve, reject);
  });
};

///////////////////////////////////////////////////////////////////////////////
// Service Management
///////////////////////////////////////////////////////////////////////////////

function handleServiceAdded(msg) {
  var service = new TelephonyService(msg.service);
  var evt = new CustomEvent('serviceadded');
  addReadonlyProperty(evt, 'serviceId', service.serviceId);
  _telephonyManager.dispatchEvent(evt);
  if (typeof _telephonyManager.onserviceadded == 'function')
    _telephonyManager.onserviceadded(evt);
}

function handleServiceChanged(msg) {
  var evt = new CustomEvent('servicechanged');
  addReadonlyProperty(evt, 'service', new TelephonyService(msg.service));
  addReadonlyProperty(evt, 'changedProperties', msg.changedProperties);
  _telephonyManager.dispatchEvent(evt);
  if (typeof _telephonyManager.onservicechanged == 'function')
    _telephonyManager.onservicechanged(evt);
}

function handleServiceRemoved(msg) {
  var evt = new CustomEvent('serviceremoved');
  addReadonlyProperty(evt, 'service', msg.service);
  _telephonyManager.dispatchEvent(evt);
  if (typeof _telephonyManager.onserviceremoved == 'function')
    _telephonyManager.onserviceremoved(evt);
}

function notifyDefaultServiceChanged() {
  var evt = new CustomEvent('defaultservicechanged');
  addReadonlyProperty(evt, 'service', msg.service);
  _telephonyManager.dispatchEvent(evt);
  if (typeof _telephonyManager.ondefaultservicechanged == 'function') {
    _telephonyManager.ondefaultservicechanged(evt);
  }
}

function handleDefaultServiceChanged(msg) {
  forceSetProperty(_telephonyManager, 'defaultServiceId', msg.service.serviceId);
  notifyDefaultServiceChanged();
}

TelephonyManager.prototype.getServiceIds = function() {
  var msg = {
    'cmd': 'getServices'
  };
  return new Promise(function(resolve, reject) {
    sendRequest(msg, resolve, reject);
  });
};

function getServicesCallback(msg) {
  if (msg.isError) {
    rejectPromise(msg);
    return;
  }
  var arr = msg.returnValue;
  var res = [];
  for (var i = 0; i < arr.length; i++) {
    var service = arr[i];
    if (!_telephonyManager.defaultServiceId) {
      forceSetProperty(_telephonyManager, 'defaultServiceId', service.serviceId);
      notifyDefaultServiceChanged();
    }
    res.push(service.serviceId);
  }
  msg.returnValue = res;
  resolvePromise(msg);
}

TelephonyManager.prototype.setDefaultServiceId = function(serviceId) {
  var msg = {
    'cmd': 'setDefaultServiceId',
    'serviceId': serviceId
  };
  return new Promise(function(resolve, reject) {
    sendRequest(msg, resolve, reject);
  });
};

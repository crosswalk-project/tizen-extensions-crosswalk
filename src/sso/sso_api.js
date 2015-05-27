// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

///////////////////////////////////////////////////////////////////////////////
// Utility functions and objects
///////////////////////////////////////////////////////////////////////////////

var g_next_async_call_id = 0;
var g_async_calls = {};
var g_next_obj_id = 0;
var v8tools = requireNative('v8tools');

var PropertyFlag = {
  CONFIGURABLE: 1,
  ENUMERABLE: 2,
  WRITEABLE: 4
};

function AsyncCall(resolve, reject, command) {
  this.resolve = resolve;
  this.reject = reject;
  this.command = command;
}

function _createPromise(msg) {
  var promise = new Promise(function(resolve, reject) {
    g_async_calls[g_next_async_call_id] = new AsyncCall(resolve, reject,
        msg.asyncOpCmd);
  });
  msg.asyncOpId = g_next_async_call_id;
  extension.postMessage(JSON.stringify(msg));
  ++g_next_async_call_id;
  return promise;
}

function _sendSyncMessage(msg) {
  return JSON.parse(extension.internal.sendSyncMessage(JSON.stringify(msg)));
}

function _isCommandInProgress(command) {
  for (var key in g_async_calls) {
    if (g_async_calls.hasOwnProperty(key) &&
        g_async_calls[key].command == command) {
      return true;
    }
  }
  return false;
}

function _addProperty(obj, propertyKey, propertyValue, propertyMask) {
  Object.defineProperty(obj, propertyKey, {
    configurable: Boolean(propertyMask & PropertyFlag.CONFIGURABLE),
    enumerable: Boolean(propertyMask & PropertyFlag.ENUMERABLE),
    writable: Boolean(propertyMask & PropertyFlag.WRITEABLE),
    value: propertyValue
  });
}

function _addReadOnlyProperty(obj, propertyKey, propertyValue) {
  _addProperty(obj, propertyKey, propertyValue, PropertyFlag.ENUMERABLE);
}

function _addHiddenProperty(obj, propertyKey, propertyValue) {
  _addProperty(obj, propertyKey, propertyValue, PropertyFlag.WRITEABLE);
}

function _addReadWriteProperty(obj, propertyKey, propertyValue) {
  _addProperty(obj, propertyKey, propertyValue, PropertyFlag.ENUMERABLE | PropertyFlag.WRITEABLE);
}

function derive(child, parent) {
  child.prototype = Object.create(parent.prototype);
  child.prototype.constructor = child;
  _addProperty(child.prototype, 'constructor', child, PropertyFlag.WRITEABLE);
}

function EventTargetInterface(eventListeners, isValidType) {
  _addHiddenProperty(this, 'listeners', eventListeners);
  _addHiddenProperty(this, 'isValidType', isValidType);
}

EventTargetInterface.prototype.addEventListener = function(type, callback) {
  if (callback != null && typeof callback === 'function' &&
      this.isValidType == type && this.listeners[type].indexOf(callback) != -1) {
    this.listeners[type].push(callback);
  }
};

EventTargetInterface.prototype.removeEventListener = function(type, callback) {
  if (callback != null && typeof callback === 'function' &&
      this.isValidType == type) {
    var index = this.listeners[type].indexOf(callback);
    if (index >= 0)
      this.listeners[type].slice(index, 1);
  }
};

EventTargetInterface.prototype.dispatchEvent = function(event) {
  if (typeof event !== 'object' || !this.isValidType == event.type)
    return false;

  var handled = true;
  this.listeners[event.type].forEach(function(callback) {
    var res = callback(event);
    if (!res && handled)
      handled = false;
  });
  return handled;
};

function _updateIdentityProps(src, dest) {
  for (var prop in src) {
    if (src.hasOwnProperty(prop) && src[prop] && dest.hasOwnProperty(prop))
      dest[prop] = src[prop];
  }
}

function _getSessionObj(identity, sessionJsId) {
  var session = null;
  if (sessionJsId <= -1 || identity == null)
    return null;
  for (var j = 0; j < identity.sessions.length; j++) {
    if (identity.sessions[j].jsid == sessionJsId) {
      session = identity.sessions[j];
      break;
    }
  }
  return session;
}

function _getSessionObjByMethod(identity, method) {
  var session = null;
  if (typeof method !== 'string' || method.length == 0 || identity == null)
    return null;
  for (var j = 0; j < identity.sessions.length; j++) {
    if (identity.sessions[j].method == method) {
      session = identity.sessions[j];
      break;
    }
  }
  return session;
}

function _getSessionObjByJSId(sessionJsId) {
  if (sessionJsId <= -1)
    return null;
  var identities = _sso.authService.identities;
  var session = null;
  for (var j = 0; j < identities.length; j++) {
    session = _getSessionObj(identities[j], sessionJsId);
    if (session)
      break;
  }
  return session;
}

function _getIdentityObj(identities, key, value) {
  var identity = null;
  for (var i = 0; i < identities.length; i++) {
    var ident = identities[i];
    if (identities[i].hasOwnProperty(key) && identities[i][key] == value) {
      identity = identities[i];
      break;
    }
  }
  return identity;
}

function _convertToIdentType(identType) {
  if (isNaN(identType)) {
    if (identType == 'APPLICATION')
      return IdentityType.APPLICATION;
    else if (identType == 'NETWORK')
      return IdentityType.NETWORK;
  }

  return (identType == IdentityType.APPLICATION || identType == IdentityType.NETWORK) ?
      identType : IdentityType.WEB;
}

///////////////////////////////////////////////////////////////////////////////
// Enumerations and dictionaries
///////////////////////////////////////////////////////////////////////////////

var IdentityType = {
  APPLICATION: 1,
  WEB: 2,
  NETWORK: 4
};

var SessionState = {
  NOT_STARTED: 0,
  RESOLVING_HOST: 1,
  CONNECTING: 2,
  SENDING_DATA: 3,
  WAITING_REPLY: 4,
  USER_PENDING: 5,
  UI_REFRESHING: 6,
  PROCESS_PENDING: 7,
  STARTED: 8,
  PROCESS_CANCELLING: 9,
  PROCESS_DONE: 10,
  CUSTOM: 11
};

var UserPromptPolicy = {
  DEFAULT: 0,
  REQUEST_PASSWORD: 1,
  NO_USER_INTERACTION: 2,
  VALIDATION: 3
};

function IdentityInfo(info) {
  var type = _convertToIdentType((info && info.hasOwnProperty('type') && info.type) || 'WEB');
  _addReadWriteProperty(this, 'type', Number(type));
  _addReadWriteProperty(this, 'username',
      String((info && info.hasOwnProperty('username') && info.username)) || null);
  _addReadWriteProperty(this, 'caption',
      String((info && info.hasOwnProperty('caption') && info.caption)) || null);
  _addReadWriteProperty(this, 'secret',
      String((info && info.hasOwnProperty('secret') && info.secret)) || null);
  _addReadWriteProperty(this, 'storeSecret',
      Boolean((info && info.hasOwnProperty('storeSecret') && info.storeSecret) || false));
  _addReadWriteProperty(this, 'realms', (info && info.hasOwnProperty('realms') && info.realms) ||
      null);
  _addReadWriteProperty(this, 'owner', (info && info.hasOwnProperty('owner') && info.owner) ||
      null);
  _addReadWriteProperty(this, 'accessControlList',
      (info && info.hasOwnProperty('accessControlList') && info.accessControlList) || null);
}

function IdentityInfoSerialized(info, id) {
  if (info == null)
    return null;

  if (info.hasOwnProperty('type') && info.type != null)
    this.type = info.type;
  if (id != null)
    this.id = id;
  if (info.hasOwnProperty('username') && info.username != null)
    this.username = info.username;
  if (info.hasOwnProperty('caption') && info.caption != null)
    this.caption = info.caption;
  if (info.hasOwnProperty('secret') && info.secret != null)
    this.secret = info.secret;
  if (info.hasOwnProperty('storeSecret') && info.storeSecret != null)
    this.storeSecret = info.storeSecret;
  if (info.hasOwnProperty('realms') && info.realms != null)
    this.realms = info.realms;
  if (info.hasOwnProperty('owner') && info.owner != null)
    this.owner = info.owner;
  if (info.hasOwnProperty('accessControlList') &&
      info.accessControlList != null)
    this.accessControlList = info.accessControlList;
}

///////////////////////////////////////////////////////////////////////////////
// Extension msg listener and handlers
///////////////////////////////////////////////////////////////////////////////
extension.setMessageListener(function(json) {
  var msg = JSON.parse(json);
  if (msg.asyncOpErrorMsg != null) {
    handleAsyncCallError(msg);
    return;
  }

  if (msg.asyncOpCmd != null) {
    switch (msg.asyncOpCmd) {
      case 'queryMethods':
      case 'queryMechanisms':
      case 'queryIdentities':
      case 'clear':
      case 'requestCredentialsUpdate':
      case 'addReference':
      case 'removeReference':
      case 'verifyUser':
      case 'verifyUserPrompt':
      case 'signout':
      case 'cancel':
        handleAsyncCallSuccess(msg);
        break;
      case 'createIdentity':
        handleCreateIdentity(msg);
        break;
      case 'getIdentity':
        handleGetIdentity(msg);
        break;
      case 'getSession':
        handleGetSession(msg);
        break;
      case 'store':
        handleStore(msg);
        break;
      case 'remove':
        handleRemoveIdentity(msg);
        break;
      case 'queryAvailableMechanisms':
        handleQueryAvailableMechanisms(msg);
        break;
      case 'challenge':
        handleChallenge(msg);
        break;
      default:
        handleAsyncCallError(msg);
        break;
    }
  } else if (msg.info != null) {
    switch (msg.info.info) {
      case 'signedout':
        var identities = _sso.authService.identities;
        var identity = _getIdentityObj(identities, 'jsid', msg.objectJSId);
        if (identity && typeof identity.onsignedout === 'function')
          identity.onsignedout(identity);
        break;
      case 'removed':
        if (_isCommandInProgress('remove'))
          break;
        var identities = _sso.authService.identities;
        var identity = _getIdentityObj(identities, 'jsid', msg.objectJSId);
        if (identity) {
          if (typeof identity.onremoved === 'function')
            identity.onremoved(identity);
          identities.splice(identities.indexOf(identity), 1);
        }
        break;
      case 'sessionStateChanged':
        handleSessionStateChanged(msg);
        break;
      default:
        console.log('Unknown signal received: ' + JSON.stringify(msg));
        break;
    }
  }
});

function handleAsyncCallSuccess(msg) {
  g_async_calls[msg.asyncOpId].resolve(msg.responseData);
  delete g_async_calls[msg.asyncOpId];
}

function handleAsyncCallError(msg) {
  var identities = _sso.authService.identities;
  if (msg.asyncOpCmd == 'getIdentity') {
    var identity = _getIdentityObj(identities, 'jsid', msg.responseData.identityJSId);
    if (identity != null)
      identities.splice(identities.indexOf(identity), 1);
  }
  g_async_calls[msg.asyncOpId].reject(Error(msg.asyncOpErrorMsg));
  delete g_async_calls[msg.asyncOpId];
}

function handleCreateIdentity(msg) {
  var identities = _sso.authService.identities;
  var identity = _getIdentityObj(identities, 'jsid', msg.responseData.identityJSId);
  if (identity != null) {
    g_async_calls[msg.asyncOpId].resolve(identity);
  } else {
    g_async_calls[msg.asyncOpId].reject(Error('Identity object not found for create Identity'));
  }
  delete g_async_calls[msg.asyncOpId];
}

function handleGetIdentity(msg) {
  var identities = _sso.authService.identities;
  var identity = _getIdentityObj(identities, 'jsid', msg.responseData.identityJSId);
  if (identity instanceof Identity) {
    _updateIdentityProps(new IdentityInfo(msg.responseData.info), identity);
    v8tools.forceSetProperty(identity, 'id', msg.responseData.info.id);
    g_async_calls[msg.asyncOpId].resolve(identity);
  } else {
    g_async_calls[msg.asyncOpId].reject(Error('Identity object not found for getidentity'));
  }
  delete g_async_calls[msg.asyncOpId];
}

function handleGetSession(msg) {
  var session = null;
  var identities = _sso.authService.identities;
  var identity = _getIdentityObj(identities, 'jsid', msg.objectJSId);
  if (identity != null)
    session = _getSessionObj(identity, msg.responseData.sessionJSId);
  if (session != null) {
    v8tools.forceSetProperty(session, 'state', msg.responseData.sessionState);
    g_async_calls[msg.asyncOpId].resolve(session);
  } else {
    g_async_calls[msg.asyncOpId].reject(Error('Identity object not found for session'));
  }
  delete g_async_calls[msg.asyncOpId];
}

function handleStore(msg) {
  var identities = _sso.authService.identities;
  var identity = _getIdentityObj(identities, 'jsid', msg.objectJSId);
  if (identity != null) {
    v8tools.forceSetProperty(identity, 'id', msg.responseData.identityId);
    g_async_calls[msg.asyncOpId].resolve(identity);
  } else {
    g_async_calls[msg.asyncOpId].reject(Error('Identity NOT FOUND'));
  }
  delete g_async_calls[msg.asyncOpId];
}

function handleRemoveIdentity(msg) {
  var identities = _sso.authService.identities;
  var identity = _getIdentityObj(identities, 'jsid', msg.objectJSId);
  if (identity != null) {
    g_async_calls[msg.asyncOpId].resolve(identity);
    identities.splice(identities.indexOf(identity), 1);
  } else {
    g_async_calls[msg.asyncOpId].reject(Error('Identity NOT FOUND'));
  }
  delete g_async_calls[msg.asyncOpId];
}

function handleSessionStateChanged(msg) {
  var session = _getSessionObjByJSId(msg.objectJSId);
  if (session == null) return;

  var event = new CustomEvent('statechanged');
  v8tools.forceSetProperty(session, 'state', msg.info.object.sessionState);
  _addReadOnlyProperty(event, 'session', session);
  if (msg.message) _addReadOnlyProperty(event, 'message', msg.message);
  session.dispatchEvent(event);
  if (session.onstatechanged)
    session.onstatechanged(event);
}

function handleQueryAvailableMechanisms(msg) {
  g_async_calls[msg.asyncOpId].resolve(msg.responseData.mechanisms);
  delete g_async_calls[msg.asyncOpId];
}

function handleChallenge(msg) {
  var data = JSON.parse(msg.responseData.sessionData);
  g_async_calls[msg.asyncOpId].resolve(data);
  delete g_async_calls[msg.asyncOpId];
}

///////////////////////////////////////////////////////////////////////////////
// AuthService
///////////////////////////////////////////////////////////////////////////////

function AuthService() {
  _addHiddenProperty(this, 'jsid', ++g_next_obj_id);
  _addHiddenProperty(this, 'identities', []);
}

AuthService.prototype.createIdentity = function(info) {
  if (typeof info !== 'object')
    return null;

  var identity = new Identity(info, this.jsid);
  if (!(identity instanceof Identity))
    return null;

  var msg = {
    'asyncOpCmd': 'createIdentity',
    'serviceJSId': this.jsid,
    'identityJSId': identity.jsid
  };
  this.identities.push(identity);
  return _createPromise(msg);
};

AuthService.prototype.getIdentity = function(id) {
  if (isNaN(id))
    return null;

  var identity = _getIdentityObj(this.identities, 'id', id);
  if (!(identity instanceof Identity)) {
    identity = new Identity(null, this.jsid);
    this.identities.push(identity);
  }
  var msg = {
    'asyncOpCmd': 'getIdentity',
    'serviceJSId': this.jsid,
    'identityId': id,
    'identityJSId': identity.jsid
  };
  return _createPromise(msg);
};

AuthService.prototype.queryMethods = function() {
  var msg = {
    'asyncOpCmd': 'queryMethods',
    'serviceJSId': this.jsid
  };
  return _createPromise(msg);
};

AuthService.prototype.queryMechanisms = function(method) {
  if (typeof method !== 'string' || method.length == 0)
    return null;

  var msg = {
    'asyncOpCmd': 'queryMechanisms',
    'serviceJSId': this.jsid,
    'method': method
  };
  return _createPromise(msg);
};

AuthService.prototype.queryIdentities = function(filter) {
  if (typeof filter !== 'object')
    return null;

  for (var key in filter) {
    if (filter.hasOwnProperty(key)) {
      if (key !== 'Type' && key !== 'Owner' && key !== 'Caption') {
        return null;
      }
    }
  }
  filter['Type'] = _convertToIdentType(filter['Type']);

  var msg = {
    'asyncOpCmd': 'queryIdentities',
    'serviceJSId': this.jsid,
    'filter': filter
  };
  return _createPromise(msg);
};

AuthService.prototype.clear = function() {
  var msg = {
    'asyncOpCmd': 'clear',
    'serviceJSId': this.jsid
  };
  return _createPromise(msg);
};

///////////////////////////////////////////////////////////////////////////////
// Identity
///////////////////////////////////////////////////////////////////////////////
function Identity(info, service_jsid) {

  IdentityInfo.call(this, info);

  _addHiddenProperty(this, 'jsid', ++g_next_obj_id);
  _addHiddenProperty(this, 'sessions', []);
  _addReadOnlyProperty(this, 'id', 0);
  _addReadWriteProperty(this, 'onsignedout', null);
  _addReadWriteProperty(this, 'onremoved', null);

  _addHiddenProperty(this, 'tracker', v8tools.lifecycleTracker());
  var jsid = this.jsid;
  this.tracker.destructor = function() {
    var msg = {
      'syncOpCmd': 'destroyIdentity',
      'serviceJSId': service_jsid,
      'identityJSId': jsid
    };
    _sendSyncMessage(msg);
  };
}
derive(Identity, IdentityInfo);

Identity.prototype.getSession = function(method) {
  if (typeof method !== 'string' || method.length == 0)
    return null;

  var session = _getSessionObjByMethod(this, method);
  if (session == null) {
    session = new AuthSession(method, SessionState.NOT_STARTED, this.jsid);
    this.sessions.push(session);
  }

  if (!(session instanceof AuthSession))
    return null;
  var msg = {
    'asyncOpCmd': 'getSession',
    'identityJSId': this.jsid,
    'sessionJSId': session.jsid,
    'method': method
  };
  return _createPromise(msg);
};

Identity.prototype.requestCredentialsUpdate = function(message) {
  if (typeof message !== 'string')
    return null;

  var msg = {
    'asyncOpCmd': 'requestCredentialsUpdate',
    'identityJSId': this.jsid,
    'message': message
  };
  return _createPromise(msg);
};

Identity.prototype.store = function() {
  var msg = {
    'asyncOpCmd': 'store',
    'identityJSId': this.jsid,
    'info': new IdentityInfoSerialized(this)
  };
  return _createPromise(msg);
};

Identity.prototype.storeWithInfo = function(info) {
  if (typeof info !== 'object')
    return null;

  _updateIdentityProps(new IdentityInfo(info), this);
  var msg = {
    'asyncOpCmd': 'store',
    'identityJSId': this.jsid,
    'info': new IdentityInfoSerialized(this)
  };
  return _createPromise(msg);
};

Identity.prototype.addReference = function(reference) {
  if (typeof reference !== 'string' || reference.length == 0)
    return null;

  var msg = {
    'asyncOpCmd': 'addReference',
    'identityJSId': this.jsid,
    'reference': reference
  };
  return _createPromise(msg);
};

Identity.prototype.removeReference = function(reference) {
  if (typeof reference !== 'string' || reference.length == 0)
    return null;

  var msg = {
    'asyncOpCmd': 'removeReference',
    'identityJSId': this.jsid,
    'reference': reference
  };
  return _createPromise(msg);
};

Identity.prototype.verifyUser = function(message) {
  if (typeof message !== 'string')
    return null;

  var msg = {
    'asyncOpCmd': 'verifyUser',
    'identityJSId': this.jsid,
    'message': message
  };
  return _createPromise(msg);
};

Identity.prototype.verifyUserPrompt = function(params) {
  if (typeof params !== 'object')
    return;

  var msg = {
    'asyncOpCmd': 'verifyUserPrompt',
    'identityJSId': this.jsid,
    'params': params
  };
  return _createPromise(msg);
};

Identity.prototype.removeIdentity = function() {
  var msg = {
    'asyncOpCmd': 'remove',
    'identityJSId': this.jsid
  };
  return _createPromise(msg);
};

Identity.prototype.signout = function() {
  var msg = {
    'asyncOpCmd': 'signout',
    'identityJSId': this.jsid
  };
  return _createPromise(msg);
};

///////////////////////////////////////////////////////////////////////////////
// AuthSession
///////////////////////////////////////////////////////////////////////////////
function AuthSession(method, state, identity_jsid) {
  var listeners = {};
  listeners['statechanged'] = [];
  EventTargetInterface.call(this, listeners, 'statechanged');

  _addHiddenProperty(this, 'jsid', ++g_next_obj_id);
  _addHiddenProperty(this, 'identityJSId', identity_jsid);
  _addReadOnlyProperty(this, 'method', method);
  _addReadOnlyProperty(this, 'state', state);
  _addReadWriteProperty(this, 'onstatechanged', null);

  _addHiddenProperty(this, 'tracker', v8tools.lifecycleTracker());
  var jsid = this.jsid;
  this.tracker.destructor = function() {
    var msg = {
      'syncOpCmd': 'destroySession',
      'sessionJSId': jsid,
      'identityJSId': identity_jsid
    };
    _sendSyncMessage(msg);
  };
}
derive(AuthSession, EventTargetInterface);

AuthSession.prototype.queryAvailableMechanisms = function(wantedMechanisms) {
  if (typeof wantedMechanisms !== 'object')
    return;

  var msg = {
    'asyncOpCmd': 'queryAvailableMechanisms',
    'sessionJSId': this.jsid,
    'wantedMechanisms': wantedMechanisms
  };
  return _createPromise(msg);
};

AuthSession.prototype.challenge = function(mechanism, sessionData) {
  if (typeof mechanism !== 'string' || typeof sessionData !== 'object')
    return;

  var msg = {
    'asyncOpCmd': 'challenge',
    'sessionJSId': this.jsid,
    'mechanism': mechanism,
    'sessionData': sessionData
  };
  return _createPromise(msg);
};

AuthSession.prototype.cancel = function() {
  var msg = {
    'asyncOpCmd': 'cancel',
    'sessionJSId': this.jsid
  };
  return _createPromise(msg);
};

///////////////////////////////////////////////////////////////////////////////
// Exports
///////////////////////////////////////////////////////////////////////////////
function SSO() {
  this.authService = new AuthService();
}
var _sso = new SSO();
exports = _sso;

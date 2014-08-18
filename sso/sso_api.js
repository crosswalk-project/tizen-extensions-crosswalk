// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

///////////////////////////////////////////////////////////////////////////////
// Utilities
///////////////////////////////////////////////////////////////////////////////

var g_next_async_call_id = 0;
var g_async_calls = {};
var g_next_obj_id = 0;

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

function _addConstProperty(obj, propertyKey, propertyValue) {
  Object.defineProperty(obj, propertyKey, {
    configurable: true,
    enumerable: true,
    writable: false,
    value: propertyValue
  });
}

function _addConstPropertyFromObject(obj, propertyKey, propObject) {
  if (!propObject.hasOwnProperty(propertyKey))
    return;
  Object.defineProperty(obj, propertyKey, {
    configurable: false,
    enumerable: true,
    writable: false,
    value: propObject[propertyKey]});
}

function _addPropertyFromObject(obj, propertyKey, propObject) {
  if (!propObject.hasOwnProperty(propertyKey))
    return;
  Object.defineProperty(obj, propertyKey, {
    configurable: false,
    enumerable: true,
    writable: true,
    value: propObject[propertyKey]});
}

///////////////////////////////////////////////////////////////////////////////
// Enumerations and dictionaries
///////////////////////////////////////////////////////////////////////////////

var IdentityType = {
  APPLICATION: 0,
  WEB: 1,
  NETWORK: 2
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

function MechanismQueryResult(obj) {
  _addConstPropertyFromObject(this, 'method', obj);
  _addConstPropertyFromObject(this, 'mechanisms', obj);
}

function IdentityInfo(obj, id, type, owner) {
  _addConstProperty(this, 'id', id);
  _addConstProperty(this, 'type', type);
  _addPropertyFromObject(this, 'username', obj);
  _addPropertyFromObject(this, 'caption', obj);
  _addPropertyFromObject(this, 'secret', obj);
  _addPropertyFromObject(this, 'storeSecret', obj);
  _addPropertyFromObject(this, 'realms', obj);
  _addConstProperty(this, 'owner', owner);
  _addPropertyFromObject(this, 'accessControlList', obj);
}

function _destroyIdentity(ident) {
  var msg = {
    'asyncOpCmd': 'destroyIdentity',
    'serviceJSId': g_auth_service.jsid,
    'identityJSId': ident.jsid
  };
  return _createPromise(msg);
}

function _getSessionObj(sessionJSId) {
  var sessobj = null;
  if (sessionJSId == -1) return null;
  for (var i = 0; i < g_identities.length; i++) {
    var identity = g_identities[i];
    for (var j = 0; j < identity.sessions.length; j++) {
      var sess = identity.sessions[j];
      if (sess.jsid == sessionJSId) {
        sessobj = sess;
        break;
      }
    }
  }
  return sessobj;
}

function _getIdentityObj(id) {
  if (id == -1) return null;
  var identobj = null;
  for (var i = 0; i < g_identities.length; i++) {
    var curr_ident = g_identities[i];
    if (curr_ident.info.id == id) {
      identobj = curr_ident;
      break;
    }
  }
  return identobj;
}

function _getIdentityObjByJSId(jsid) {
  if (jsid == -1) return null;
  var identobj = null;
  for (var i = 0; i < g_identities.length; i++) {
    var curr_ident = g_identities[i];
    if (curr_ident.jsid == jsid) {
      identobj = curr_ident;
      break;
    }
  }
  return identobj;
}

function _convertToIdentType(stringtype) {
  if (stringtype == 'APPLICATION') return IdentityType.APPLICATION;
  else if (stringtype == 'WEB') return IdentityType.WEB;
  else if (stringtype == 'NETWORK') return IdentityType.NETWORK;
  else return IdentityType.WEB;
}

///////////////////////////////////////////////////////////////////////////////
// Extension msg listener and utility functions
///////////////////////////////////////////////////////////////////////////////
var g_identities = [];
var g_auth_service = new AuthService();
exports.authService = g_auth_service;

var g_auth_session_listeners = {};
g_auth_session_listeners['statechanged'] = [];

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
      case 'destroyIdentity':
      case 'requestCredentialsUpdate':
      case 'addReference':
      case 'removeReference':
      case 'verifyUser':
      case 'verifyUserPrompt':
      case 'signout':
      case 'cancel':
        handleAsyncCallSuccess(msg);
        break;
      case 'getIdentity':
        handleGetIdentity(msg);
        break;
      case 'startSession':
        handleStartSession(msg);
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
        var identobj = g_auth_service.getIdentityByJSId(msg.objectJSId);
        if (identobj != null && identobj.onsignedout) identobj.onsignedout(identobj);
        break;
      case 'removed':
        if (_isCommandInProgress('remove'))
          break;
        var identobj = g_auth_service.getIdentityByJSId(msg.objectJSId);
        if (identobj != null) {
          if (identobj.onremoved) identobj.onremoved(identobj);
          g_identities.splice(g_identities.indexOf(identobj), 1);
          _destroyIdentity(identobj);
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
  if (msg.asyncOpCmd == 'getIdentity') {
    var identobj = g_auth_service.getIdentityByJSId(
        msg.responseData.identityJSId);
    if (identobj != null)
      g_identities.splice(g_identities.indexOf(identobj), 1);
  }
  g_async_calls[msg.asyncOpId].reject(Error(msg.asyncOpErrorMsg));
  delete g_async_calls[msg.asyncOpId];
}

function handleGetIdentity(msg) {
  var identobj = _getIdentityObjByJSId(msg.responseData.identityJSId);
  if (identobj) {
    identobj.info = new IdentityInfo(msg.responseData.info,
        msg.responseData.info.id, _convertToIdentType(msg.responseData.info.type),
        msg.responseData.info.owner);
    g_async_calls[msg.asyncOpId].resolve(identobj);
  } else {
    g_async_calls[msg.asyncOpId].reject(Error('Identity object not found for getidentity'));
  }
  delete g_async_calls[msg.asyncOpId];
}

function handleStartSession(msg) {
  var identobj = g_auth_service.getIdentityByJSId(msg.objectJSId);
  if (identobj) {
    var session = new AuthSession(msg.responseData, msg.objectJSId);
    identobj.sessions.push(session);
    g_async_calls[msg.asyncOpId].resolve(session);
  } else {
    g_async_calls[msg.asyncOpId].reject(Error('Identity object not found for session'));
  }
  delete g_async_calls[msg.asyncOpId];
}

function handleStore(msg) {
  var identobj = g_auth_service.getIdentityByJSId(msg.objectJSId);
  if (identobj != null) {
    var info = new IdentityInfo(identobj.info, msg.responseData.identityId,
        identobj.info.type, identobj.info.owner);
    identobj.info = info;
    g_async_calls[msg.asyncOpId].resolve(identobj);
  } else {
    g_async_calls[msg.asyncOpId].reject(Error('Identity NOT FOUND'));
  }
  delete g_async_calls[msg.asyncOpId];
}

function handleRemoveIdentity(msg) {
  var identobj = g_auth_service.getIdentityByJSId(msg.objectJSId);
  if (identobj != null) {
    g_async_calls[msg.asyncOpId].resolve(identobj);
    g_identities.splice(g_identities.indexOf(identobj), 1);
  } else {
    g_async_calls[msg.asyncOpId].reject(Error('Identity NOT FOUND'));
  }
  delete g_async_calls[msg.asyncOpId];
}

function handleSessionStateChanged(msg) {
  var session = _getSessionObj(msg.objectJSId);
  if (session == null) return;

  var event = new CustomEvent('statechanged');
  session.sessionState = msg.info.object.sessionState;
  _addConstProperty(event, 'session', session);
  if (msg.message) _addConstProperty(event, 'message', msg.message);
  session.dispatchEvent(event);
  if (session.onstatechanged)
    session.onstatechanged(event);
}

function handleQueryAvailableMechanisms(msg) {
  g_async_calls[msg.asyncOpId].resolve(msg.responseData.mechanisms);
  delete g_async_calls[msg.asyncOpId];
}

function handleChallenge(msg) {
  g_async_calls[msg.asyncOpId].resolve(msg.responseData.sessionData);
  delete g_async_calls[msg.asyncOpId];
}

///////////////////////////////////////////////////////////////////////////////
// AuthService
///////////////////////////////////////////////////////////////////////////////

function AuthService() {
  this.jsid = ++g_next_obj_id;
}

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

AuthService.prototype.getIdentity = function(identity_id) {
  if (isNaN(identity_id))
    return null;

  var identobj = _getIdentityObj(identity_id);
  if (identobj == null) {
    identobj = new Identity(null);
    g_identities.push(identobj);
  }
  var msg = {
    'asyncOpCmd': 'getIdentity',
    'serviceJSId': this.jsid,
    'identityId': identity_id,
    'identityJSId': identobj.jsid
  };
  return _createPromise(msg);
};

AuthService.prototype.getIdentityByJSId = function(jsid) {
  if (isNaN(jsid))
    return null;

  return _getIdentityObjByJSId(jsid);
};

AuthService.prototype.createIdentity = function(info) {
  if (typeof info !== 'object')
    return null;

  var identobj = new Identity(new IdentityInfo(info, info.id,
      _convertToIdentType(info.type), info.owner));
  var msg = {
    'syncOpCmd': 'createIdentity',
    'serviceJSId': this.jsid,
    'identityJSId': identobj.jsid
  };
  var res = _sendSyncMessage(msg);
  if (res.syncOpErrorMsg != null) {
    return res;
  }
  g_identities.push(identobj);
  res.identity = identobj;
  return res;
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
function Identity(info) {
  this.onsignedout = null;
  this.onremoved = null;
  this.info = info;
  this.sessions = [];
  this.jsid = ++g_next_obj_id;
}

Identity.prototype.startSession = function(method) {
  if (typeof method !== 'string' || method.length == 0)
    return null;

  var msg = {
    'asyncOpCmd': 'startSession',
    'identityJSId': this.jsid,
    'sessionJSId': ++g_next_obj_id,
    'method': method
  };
  return _createPromise(msg);
};

Identity.prototype.getSessionByJSId = function(jsid) {
  if (isNaN(jsid) || jsid == -1)
    return null;

  var sessobj = null;
  for (var i = 0; i < this.sessions.length; i++) {
    var sess = this.sessions[i];
    if (sess.jsid == jsid) {
      sessobj = sess;
      break;
    }
  }
  return sessobj;
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
    'info': this.info
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

Identity.prototype.remove = function() {
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

Identity.prototype.updateInfo = function(info) {
  if (typeof info !== 'object')
    return;

  var obj = null;
  if (this.info)
    obj = new IdentityInfo(info, this.info.id, this.info.type, this.info.owner);
  else
    obj = new IdentityInfo(info, info.id, _convertToIdentType(info.type),
                           info.owner);
  this.info = obj;
};

///////////////////////////////////////////////////////////////////////////////
// AuthSession
///////////////////////////////////////////////////////////////////////////////
function AuthSession(obj, identity_jsid) {
  _addConstProperty(this, 'jsid', obj.sessionJSId);
  _addConstProperty(this, 'identityJSId', identity_jsid);
  _addConstPropertyFromObject(this, 'method', obj);
  _addPropertyFromObject(this, 'sessionState', obj);
  this.onstatechanged = null;
}

function isValidType(type) {
  return (type === 'statechanged');
}

AuthSession.prototype.addEventListener = function(type, callback) {
  if (callback != null && isValidType(type)) {
    if (g_auth_session_listeners[type].indexOf(callback) != -1)
      g_auth_session_listeners[type].push(callback);
  }
};

AuthSession.prototype.removeEventListener = function(type, callback) {
  if (callback != null && isValidType(type)) {
    var index = g_auth_session_listeners[type].indexOf(callback);
    if (index >= 0)
      g_auth_session_listeners[type].splice(index, 1);
  }
};

AuthSession.prototype.dispatchEvent = function(event) {
  var handled = true;
  if (typeof event !== 'object' || !isValidType(event.type))
    return false;

  g_auth_session_listeners[event.type].forEach(function(callback) {
    var res = callback(event);
    if (!res && handled) handled = false;
  });
  return handled;
};

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

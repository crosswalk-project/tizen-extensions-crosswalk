/****************************************************************************
**
** Copyright © 1992-2014 Cisco and/or its affiliates. All rights reserved.
** All rights reserved.
**
** $CISCO_BEGIN_LICENSE:APACHE$
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
** http://www.apache.org/licenses/LICENSE-2.0
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
** $CISCO_END_LICENSE$
**
****************************************************************************/


var _debug_enabled = 1;  // toggle this to show/hide debug logs

///////////////////////////////////////////////////////////////////////////////
// Logging helpers
///////////////////////////////////////////////////////////////////////////////
function DBG(msg) {
  if (!_debug_enabled) return;
  console.log('[Iotivity-D]:' + msg);
}

function ERR(msg) {
  console.error('[Iotivity-E]:' + msg);
}

///////////////////////////////////////////////////////////////////////////////
// Utilities
///////////////////////////////////////////////////////////////////////////////

var g_next_async_call_id = 0;
var g_async_calls = {};

function AsyncCall(resolve, reject) {
  this.resolve = resolve;
  this.reject = reject;
}

function createPromise(msg) {
  var promise = new Promise(function(resolve, reject) {
    g_async_calls[g_next_async_call_id] = new AsyncCall(resolve, reject);
  });
  msg.asyncCallId = g_next_async_call_id;
  extension.postMessage(JSON.stringify(msg));
  ++g_next_async_call_id;
  return promise;
}

function _addConstProperty(obj, propertyKey, propertyValue) {
  Object.defineProperty(obj, propertyKey, {
    configurable: true,
    writable: false,
    value: propertyValue
  });
}

function _addProperty(obj, propertyKey, propertyValue) {
  Object.defineProperty(obj, propertyKey, {
    configurable: true,
    writable: true,
    enumerable: true,
    value: propertyValue
  });
}

function _addConstructorProperty(obj, constructor) {
  Object.defineProperty(obj, 'constructor', {
    enumerable: false,
    value: constructor
  });
}

function _addConstPropertyFromObject(obj, propertyKey, propObject) {
  if (propObject.hasOwnProperty(propertyKey)) {
    Object.defineProperty(obj, propertyKey, {
      configurable: true,
      writable: false,
      value: propObject[propertyKey]
    });
  }
}

function derive(child, parent) {
  child.prototype = Object.create(parent.prototype);
  child.prototype.constructor = child;
  _addConstructorProperty(child.prototype, child);
}


///////////////////////////////////////////////////////////////////////////////
// EventTarget
///////////////////////////////////////////////////////////////////////////////

  function EventTarget(event_types) {
    var _event_listeners = { };
    var _event_handlers = { };
    var self = this;

    if (event_types != null) {
      // initialize event listeners
      event_types.forEach(function(type) {
        _event_listeners[type] = [];
        // Setup a Event Handler
        _addProperty(self, 'on' + type, null);
      });
    }

    this._event_listeners = _event_listeners;
  }

  EventTarget.prototype.isValidEventType = function(type) {
    return type in this._event_listeners;
  };

  EventTarget.prototype.addEventListener = function(type, listener) {
    if (this.isValidEventType(type) &&
        listener != null && typeof listener === 'function' &&
        this._event_listeners[type].indexOf(listener) == -1) {
      this._event_listeners[type].push(listener);
      return;
    }
    ERR('Invalid event type \'' + type + '\', ignoring.' +
        'avaliable event types : ' + this._event_listeners);
  };

  EventTarget.prototype.removeEventListener = function(type, listener) {
    if (!type || !listener)
      return;

    var index = this._event_listeners[type].indexOf(listener);
    if (index == -1)
      return;

    this._event_listeners[type].splice(index, 1);
  };

  EventTarget.prototype.dispatchEvent = function(ev) {
    var handled = true;
    var listeners = null;
    if (typeof ev === 'object' &&
        this.isValidEventType(ev.type) &&
        (listeners = this._event_listeners[ev.type]) != null) {
      listeners.forEach(function(listener) {
        if (!listener(ev) && handled)
          handled = false;
      });
    }
    return handled;
  };

///////////////////////////////////////////////////////////////////////////////
// Enum
///////////////////////////////////////////////////////////////////////////////
var OicDeviceRole = {
   "client":1, 
   "server":2,
   "intermediary":3
};

var OicConnectionMode { 
    "acked":1, 
    "non-acked":2, 
    “default”:3
}; // default is either of the former

var OicMethod = {
    "init":1, 
    "observe”:2, 
    "create”:3, 
    "retrieve”:4, 
    "update”:5, 
    "delete”:6
};


///////////////////////////////////////////////////////////////////////////////
// OicDevice
///////////////////////////////////////////////////////////////////////////////
function OicDevice(settings, events) {
  events.push('OicDevice');
  EventTarget.call(this, events);

  _addConstProperty(this, 'settings', settings);
  _addConstProperty(this, 'OicClient', OicClient);
  _addConstProperty(this, 'OicServer', OicServer);
}

OicDevice.prototype.configure = function(settings) {
  var msg = {
    'cmd': 'configure',
    'settings': settings
  };
  return createPromise(msg);
};

///////////////////////////////////////////////////////////////////////////////
// OicDeviceSettings
///////////////////////////////////////////////////////////////////////////////

function OicDeviceSettings(obj) {

  _addConstProperty(this, 'url', obj.url);
  _addConstProperty(this, 'OicDeviceInfo', obj.info);
  _addConstProperty(this, 'OicDeviceRole', obj.role);
  _addConstProperty(this, 'OicConnectionMode', obj.connectionMode);
}

///////////////////////////////////////////////////////////////////////////////
// OicDeviceInfo
///////////////////////////////////////////////////////////////////////////////

function OicDeviceInfo(obj) {

  _addConstProperty(this, 'uuid', obj.uuid);
  _addConstProperty(this, 'name', obj.name);
  _addConstProperty(this, 'dataModels', obj.dataModels);
  _addConstProperty(this, 'coreSpecVersion', obj.coreSpecVersion);
  _addConstProperty(this, 'model', obj.model);
  _addConstProperty(this, 'manufacturerName', obj.manufacturerName);
  _addConstProperty(this, 'manufacturerUrl', obj.manufacturerUrl);
  _addConstProperty(this, 'manufacturerDate', obj.manufacturerDate);
  _addConstProperty(this, 'platformVersion', obj.platformVersion);
  _addConstProperty(this, 'firmwareVersion', obj.firmwareVersion);
  _addConstProperty(this, 'supportUrl', obj.supportUrl);
}

///////////////////////////////////////////////////////////////////////////////
// OicClient
///////////////////////////////////////////////////////////////////////////////

function OicClient(obj) {
  events.push('OicClient');
  EventTarget.call(this, events);

  this.onresourcechange = null;
}

// client API: discovery
OicClient.prototype.findResources = function(options) {
  var msg = {
    'cmd': 'findResources',
    'OicDiscoveryOptions': options
  };
  return createPromise(msg);
};

// client API: CRUDN
OicClient.prototype.findDevices = function(options) {
  var msg = {
    'cmd': 'findDevices',
    'OicDiscoveryOptions': options
  };
  return createPromise(msg);
};

OicClient.prototype.createResource = function(resource) {
  var msg = {
    'cmd': 'createResource',
    'param': resource
  };
  return createPromise(msg);
};

OicClient.prototype.retrieveResource = function(resourceId) {
  var msg = {
    'cmd': 'retrieveResource',
    'param': resourceId
  };
  return createPromise(msg);
};

OicClient.prototype.updateResource = function(resource) {
  var msg = {
    'cmd': 'updateResource',
    'param': resource
  };
  return createPromise(msg);
};

OicClient.prototype.deleteResource = function(resourceId) {
  var msg = {
    'cmd': 'deleteResource',
    'param': resourceId
  };
  return createPromise(msg);
};

OicClient.prototype.startObserving = function(resourceId) {
  var msg = {
    'cmd': 'startObserving',
    'param': resourceId
  };
  return createPromise(msg);
};

OicClient.prototype.cancelObserving = function(resourceId) {
  var msg = {
    'cmd': 'cancelObserving',
    'param': resourceId
  };
  return createPromise(msg);
};


///////////////////////////////////////////////////////////////////////////////
// OicServer
///////////////////////////////////////////////////////////////////////////////

function OicServer(obj) {
  events.push('OicServer');
  EventTarget.call(this, events);

  this.onrequest = null;
}

// register/unregister locally constructed resource objects with OIC
// gets an id
OicClient.prototype.registerResource = function(init) {
  var msg = {
    'cmd': 'registerResource',
    'OicResourceInit': init
  };
  return createPromise(msg);
};

// purge resource, then notify all
OicClient.prototype.unregisterResource = function(resourceId) {
  var msg = {
    'cmd': 'unregisterResource',
    'resourceId': resourceId
  };
  return createPromise(msg);
};

// enable/disable presence (discovery, state changes) for this device and its resources
OicClient.prototype.enablePresence = function() {
  var msg = {
    'cmd': 'enablePresence'
  };
  return createPromise(msg);
};

OicClient.prototype.disablePresence = function() {
  var msg = {
    'cmd': 'registerResource'
  };
  return createPromise(msg);
};

OicClient.prototype.notify = function(resourceId, method, updatedPropertyNames) {
  var msg = {
    'cmd': 'notify',
    'resourceId': resourceId,
    'method': method,
    'updatedPropertyNames': updatedPropertyNames
  };
  return createPromise(msg);
};

///////////////////////////////////////////////////////////////////////////////
// OicRequestEvent
///////////////////////////////////////////////////////////////////////////////

function OicRequestEvent() {

  Event.call(this);
/*
  _addConstProperty(this, 'type', obj.type);
  _addConstProperty(this, 'requestId', obj.requestId);
  _addConstProperty(this, 'source', obj.source);
  _addConstProperty(this, 'target', obj.target);
  _addConstProperty(this, 'res', obj.res);
  _addConstProperty(this, 'updatedPropertyNames', obj.updatedPropertyNames);
  _addConstProperty(this, 'headerOptions', obj.headerOptions);
  _addConstProperty(this, 'queryOptions', obj.queryOptions);
*/
}

// for create, observe, retrieve
// reuses request info (type, requestId, source, target) to construct response,
// sends back “ok”, plus the resource object if applicable
OicRequestEvent.prototype.sendResponse = function(resource) {
  var msg = {
    'cmd': 'sendResponse',
    'param': resource
  };
  return createPromise(msg);
};

// reuses request info (type, requestId, source, target) to construct response,
// sends an Error, where error.message maps to OIC errors, see later
OicRequestEvent.prototype.sendError = function(error) {
  var msg = {
    'cmd': 'sendError',
    'param': error
  };
  return createPromise(msg);
};

///////////////////////////////////////////////////////////////////////////////
// OicResourceChangedEvent
///////////////////////////////////////////////////////////////////////////////

function OicResourceChangedEvent(obj) {

  Event.call(this);

  _addConstProperty(this, 'type', obj.type);
  _addConstProperty(this, 'resource', obj.resource);
  _addConstProperty(this, 'updatedPropertyNames', obj.updatedPropertyNames);resource
}

///////////////////////////////////////////////////////////////////////////////
// OicDiscoveryOptions, OicResourceRepresentation, OicResourceInit
///////////////////////////////////////////////////////////////////////////////

// all properties are null by default, meaning “find all
// if resourceId is specified in full form, a direct retrieve is made
// if resourceType is specified, a retrieve on /oic/res is made
// if resourceId is null, and deviceId not, then only resources from that device are returned
function OicDiscoveryOptions(obj) {

  _addConstProperty(this, 'deviceId', obj.deviceId);
  _addConstProperty(this, 'resourceId', obj.resourceId);
  _addConstProperty(this, 'resourceType', obj.resourceType);
}

function OicResourceRepresentation(obj) {

// any non-function properties that are JSON-serializable, e.g. string, number, boolean, URI
}

function OicResourceInit(obj) {

  _addConstProperty(this, 'url', obj.url);
  _addConstProperty(this, 'deviceId', obj.deviceId);
  _addConstProperty(this, 'connectionMode', obj.connectionMode);
  _addConstProperty(this, 'resourceTypes', obj.resourceTypes);
  _addConstProperty(this, 'interfaces', obj.interfaces);
  _addConstProperty(this, 'discoverable', obj.discoverable);
  _addConstProperty(this, 'observable', obj.observable);

  // resource hierarchies, perhaps not needed in the first version
  _addConstProperty(this, 'parent', obj.parent);
  _addConstProperty(this, 'children', obj.children);

  // additional, resource type specific properties are allowed as “mixin”
  _addConstProperty(this, 'properties', obj.properties);
}

///////////////////////////////////////////////////////////////////////////////
// OicResource
///////////////////////////////////////////////////////////////////////////////
function OicResource(obj) {

  // id: obtained when registered, empty at construction

  // properties of OicResourceInit are exposed as readonly attributes
  _addConstProperty(this, 'url', obj.url);
  _addConstProperty(this, 'deviceId', obj.deviceId);
  _addConstProperty(this, 'connectionMode', obj.connectionMode);
  _addConstProperty(this, 'resourceTypes', obj.resourceTypes);
  _addConstProperty(this, 'interfaces', obj.interfaces);
  _addConstProperty(this, 'discoverable', obj.discoverable);
  _addConstProperty(this, 'observable', obj.observable);

  // resource hierarchies, perhaps not needed in the first version
  _addConstProperty(this, 'parent', obj.parent);
  _addConstProperty(this, 'children', obj.children);

  // additional, resource type specific properties are allowed as “mixin”
  _addConstProperty(this, 'properties', obj.properties);
}

///////////////////////////////////////////////////////////////////////////////
// HeaderOption 
///////////////////////////////////////////////////////////////////////////////
// see also https://fetch.spec.whatwg.org/#headers
function HeaderOption(obj) {

  _addConstProperty(this, 'name', obj.name);
  _addConstProperty(this, 'value', obj.value);
}

///////////////////////////////////////////////////////////////////////////////
// QueryOption 
///////////////////////////////////////////////////////////////////////////////
function QueryOption(obj) {

  _addConstProperty(this, 'key', obj.key);
  _addConstProperty(this, 'value', obj.value);
}


///////////////////////////////////////////////////////////////////////////////
// Exports and main entry point for the Iotivity API
///////////////////////////////////////////////////////////////////////////////

var g_iotivity_server = new OicServer();
exports = g_iotivity_server;

var g_iotivity_server = new OicClient();
exports = g_iotivity_client;

///////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////

extension.setMessageListener(function(json) {
  var msg = JSON.parse(json);
  switch (msg.cmd) {
    case 'registerResourceCompleted':
      handleRegisterResourceCompleted(msg);
      break;
    case 'entityHandler':
      handleEntityHandler(msg);
      break;

    case 'unregisterResourceCompleted':
    case 'enablePresenceCompleted':
    case 'disablePresenceCompleted':
      handleAsyncCallSuccess(msg);
      break;
    case 'asyncCallError':
      handleAsyncCallError(msg);
      break;

  }
});

function handleRegisterResourceCompleted(msg) {
  var oicResource = new OicResource(msg.OicResourceInit);
  _addConstProperty(oicResource, 'id', msg.resourceId);

  g_async_calls[msg.asyncCallId].resolve(oicResource);
}

function handleEntityHandler(msg) {
  
  if (g_iotivity_server.onrequest)
    g_iotivity_server(msg.OicRequestEvent);
}

function handleAsyncCallSuccess(msg) {
  g_async_calls[msg.asyncCallId].resolve();
}

function handleAsyncCallError(msg) {
  g_async_calls[msg.asyncCallId].reject(Error('Async operation failed'));
}


exports.handleMessageAsync = function(msg, callback) {
  iotivityListener = callback;
  extension.postMessage(msg);
};

exports.handleMessageSync = function(msg) {
  return extension.internal.sendSyncMessage(msg);
};



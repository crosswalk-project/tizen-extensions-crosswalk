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

var g_iotivity_device = null;

var asyncCallId_onresourcesfound = null;
var asyncCallId_onretrieveresource = null;
var asyncCallId_onupdateresource = null;
var asyncCallId_ondeleteresource = null;


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

/*
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
*/

///////////////////////////////////////////////////////////////////////////////
// OicDevice
///////////////////////////////////////////////////////////////////////////////
function OicDevice(settings) {

  DBG('new OicDevice: settings=' + JSON.stringify(settings));

  if (settings)
    _addConstProperty(this, 'settings', new OicDeviceSettings(settings));
  else {
    // default settings
    _addConstProperty(this, 'settings', new OicDeviceSettings(null));
  }
  _addConstProperty(this, 'client', new OicClient());
  _addConstProperty(this, 'server', new OicServer());

  g_iotivity_device = this;

  //if (g_iotivity_device)
  //  g_iotivity_device.configure(this.settings);
}

// partial dictionary is ok
// equivalent to ‘onboard+configure’ in Core spec
// maps to IoTivity Configure (C++ API), configure (Java API), OCInit (C API)
OicDevice.prototype.configure = function(settings) {

  if (settings)
    _addConstProperty(this, 'settings', new OicDeviceSettings(settings));

  var msg = {
    'cmd': 'configure',
    'settings': this.settings
  };

  return createPromise(msg);
};

// return to factory configuration and reboot
OicDevice.prototype.factoryReset = function() {
  var msg = {
    'cmd': 'factoryReset',
  };
  return createPromise(msg);
};

// keep configuration and reboot
OicDevice.prototype.reboot = function() {
  var msg = {
    'cmd': 'configure',
    'settings': settings
  };
  return createPromise(msg);
};

iotivity.OicDevice = OicDevice;

///////////////////////////////////////////////////////////////////////////////
// OicDeviceSettings
///////////////////////////////////////////////////////////////////////////////

function OicDeviceSettings(obj) {

  if (obj) {
    _addConstProperty(this, 'url', obj.url);
    _addConstProperty(this, 'info', new OicDeviceInfo(obj.info));
    _addConstProperty(this, 'role', obj.role);
    _addConstProperty(this, 'connectionMode', obj.connectionMode);
  }
  else {
    _addConstProperty(this, 'url', 'default');
    _addConstProperty(this, 'info', new OicDeviceInfo(null));
    _addConstProperty(this, 'role', 'client');
    _addConstProperty(this, 'connectionMode', 'default');
  }
}

iotivity.OicDeviceSettings = OicDeviceSettings;

///////////////////////////////////////////////////////////////////////////////
// OicDeviceInfo
///////////////////////////////////////////////////////////////////////////////

function OicDeviceInfo(obj) {

  if (obj) {
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
  else {
    _addConstProperty(this, 'uuid', 'default');
    _addConstProperty(this, 'name', 'default');
    _addConstProperty(this, 'dataModels', 'default');
    _addConstProperty(this, 'coreSpecVersion', 'default');
    _addConstProperty(this, 'model', 'default');
    _addConstProperty(this, 'manufacturerName', 'default');
    _addConstProperty(this, 'manufacturerUrl', 'default');
    _addConstProperty(this, 'manufacturerDate', 'default');
    _addConstProperty(this, 'platformVersion', 'default');
    _addConstProperty(this, 'firmwareVersion', 'default');
    _addConstProperty(this, 'supportUrl', 'default');
  }
}

iotivity.OicDeviceInfo = OicDeviceInfo;

///////////////////////////////////////////////////////////////////////////////
// OicClient
///////////////////////////////////////////////////////////////////////////////

function OicClient(obj) {
  //events.push('OicClient');
  //EventTarget.call(this, events);

  this.onresourcechange = null;
}

// client API: discovery
OicClient.prototype.findResources = function(options) {
  var msg = {
    'cmd': 'findResources',
    'OicDiscoveryOptions': options
  };

  var promise = createPromise(msg);
  asyncCallId_onresourcesfound = msg.asyncCallId;
  return promise;
};

OicClient.prototype.findDevices = function(options) {
  var msg = {
    'cmd': 'findDevices',
    'OicDiscoveryOptions': options
  };
  return createPromise(msg);
};

// client API: CRUDN
OicClient.prototype.createResource = function(resourceinit) {

  var oicRequestEvent = new OicRequestEvent();
  _addConstProperty(oicRequestEvent, 'type', 'create');

  var msg = {
    'cmd': 'createResource',
    'request': oicRequestEvent
  };
  return createPromise(msg);
};

OicClient.prototype.retrieveResource = function(resourceId) {

  var msg = {
    'cmd': 'retrieveResource',
    'resourceId': resourceId
  };

  var promise = createPromise(msg);
  asyncCallId_onretrieveresource = msg.asyncCallId;
  return promise;
};

OicClient.prototype.updateResource = function(resource) {

  var msg = {
    'cmd': 'updateResource',
    'OicResource': resource,
  };

  var promise = createPromise(msg);
  asyncCallId_onupdateresource = msg.asyncCallId;
  return promise;
};

OicClient.prototype.deleteResource = function(resourceId) {

  var msg = {
    'cmd': 'deleteResource',
    'resourceId': resourceId
  };

  var promise = createPromise(msg);
  asyncCallId_ondeleteresource = msg.asyncCallId;
  return promise;
};

OicClient.prototype.startObserving = function(resourceId) {

  var msg = {
    'cmd': 'startObserving',
    'resourceId': resourceId,
  };

  return createPromise(msg);
};

OicClient.prototype.cancelObserving = function(resourceId) {

  var oicRequestEvent = new OicRequestEvent();
  _addConstProperty(oicRequestEvent, 'type', 'observe');
  _addConstProperty(oicRequestEvent, 'target', resourceId);

  var msg = {
    'cmd': 'cancelObserving',
    'request': oicRequestEvent
  };
  return createPromise(msg);
};

iotivity.OicClient = OicClient;

///////////////////////////////////////////////////////////////////////////////
// OicServer
///////////////////////////////////////////////////////////////////////////////

function OicServer(obj) {
  //events.push('OicServer');
  //EventTarget.call(this, events);

  this.onrequest = null;
}

// register/unregister locally constructed resource objects with OIC
// gets an id
OicServer.prototype.registerResource = function(init) {
  var msg = {
    'cmd': 'registerResource',
    'OicResourceInit': init
  };
  return createPromise(msg);
};

// purge resource, then notify all
OicServer.prototype.unregisterResource = function(resourceId) {
  var msg = {
    'cmd': 'unregisterResource',
    'resourceId': resourceId
  };
  return createPromise(msg);
};

// enable/disable presence (discovery, state changes) for this device and its resources
OicServer.prototype.enablePresence = function() {
  var msg = {
    'cmd': 'enablePresence'
  };
  return createPromise(msg);
};

OicServer.prototype.disablePresence = function() {
  var msg = {
    'cmd': 'registerResource'
  };
  return createPromise(msg);
};

OicServer.prototype.notify = function(resourceId, method, updatedPropertyNames) {
  var msg = {
    'cmd': 'notify',
    'resourceId': resourceId,
    'method': method,
    'updatedPropertyNames': updatedPropertyNames
  };
  return createPromise(msg);
};

iotivity.OicServer = OicServer;

///////////////////////////////////////////////////////////////////////////////
// OicRequestEvent
///////////////////////////////////////////////////////////////////////////////
var g_next_request_id = 0;

function OicRequestEvent() {

  DBG('new OicRequestEvent');
  //_addConstProperty(this, 'requestId', g_next_request_id);
  _addConstProperty(this, 'source', g_iotivity_device.uuid);
  ++g_next_request_id;

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
    'resource': resource || null,
    'type': this.type,
    'requestId': this.requestId,
    'source': this.source,
    'target': this.target,
  };
  return createPromise(msg);
};

// reuses request info (type, requestId, source, target) to construct response,
// sends an Error, where error.message maps to OIC errors, see later
OicRequestEvent.prototype.sendError = function(error) {
  var msg = {
    'cmd': 'sendError',
    'error': error,
    'requestId': this.requestId,
    'source': this.source,
    'target': this.target,
  };
  return createPromise(msg);
};

iotivity.OicRequestEvent = OicRequestEvent;

///////////////////////////////////////////////////////////////////////////////
// OicResourceChangedEvent
///////////////////////////////////////////////////////////////////////////////

function OicResourceChangedEvent(obj) {

  Event.call(this);

  _addConstProperty(this, 'type', obj.type);
  _addConstProperty(this, 'resource', obj.resource);
  _addConstProperty(this, 'updatedPropertyNames', obj.updatedPropertyNames);resource
}

iotivity.OicResourceChangedEvent = OicResourceChangedEvent;

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

iotivity.OicDiscoveryOptions = OicDiscoveryOptions;

function OicResourceRepresentation(obj) {

// any non-function properties that are JSON-serializable, e.g. string, number, boolean, URI
}

iotivity.OicResourceRepresentation = OicResourceRepresentation;


function OicResourceInit(obj) {

  DBG('new OicResourceInit: obj=' + JSON.stringify(obj));

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

iotivity.OicResourceInit = OicResourceInit;

///////////////////////////////////////////////////////////////////////////////
// OicResource
///////////////////////////////////////////////////////////////////////////////
function OicResource(obj) {

  // id: obtained when registered, empty at construction
  DBG('new OicResource: obj=' + JSON.stringify(obj));
  // properties of OicResourceInit are exposed as readonly attributes
  _addConstProperty(this, 'url', obj.url);
  _addConstProperty(this, 'deviceId', obj.deviceId);
  _addConstProperty(this, 'connectionMode', obj.connectionMode);
  _addConstProperty(this, 'resourceTypes', obj.resourceTypes);
  _addConstProperty(this, 'interfaces', obj.interfaces);
  _addConstProperty(this, 'discoverable', obj.discoverable);
  _addConstProperty(this, 'observable', obj.observable);
  // resource hierarchies, perhaps not needed in the first version
  _addConstProperty(this, 'parent', obj.parent || null);
  _addConstProperty(this, 'children', obj.children || null);
  // additional, resource type specific properties are allowed as “mixin”
  _addConstProperty(this, 'properties', obj.properties);
}

iotivity.OicResource = OicResource;

///////////////////////////////////////////////////////////////////////////////
// HeaderOption 
///////////////////////////////////////////////////////////////////////////////
// see also https://fetch.spec.whatwg.org/#headers
function HeaderOption(obj) {

  _addConstProperty(this, 'name', obj.name);
  _addConstProperty(this, 'value', obj.value);
}

iotivity.HeaderOption = HeaderOption;

///////////////////////////////////////////////////////////////////////////////
// QueryOption 
///////////////////////////////////////////////////////////////////////////////
function QueryOption(obj) {

  _addConstProperty(this, 'key', obj.key);
  _addConstProperty(this, 'value', obj.value);
}

iotivity.QueryOption = QueryOption;

///////////////////////////////////////////////////////////////////////////////
// Exports and main entry point for the Iotivity API
///////////////////////////////////////////////////////////////////////////////




///////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////
extension.setMessageListener(function(json) {
  var msg = JSON.parse(json);

  DBG('setMessageListener msg=' + JSON.stringify(msg));

  DBG('msg.cmd='+msg.cmd);

  switch (msg.cmd) {
    case 'registerResourceCompleted':
      handleRegisterResourceCompleted(msg);
      break;
    case 'entityHandler':
      handleEntityHandler(msg);
      break;
    case 'foundResourceCallback':
      handleFoundResources(msg);
      break;
    case 'onObserve':
      handleOnObserve(msg);
      break;

    case 'retrieveResourceCompleted':
      handleRetrieveResource(msg);
      break;

    case 'updateResourceCompleted':
      handleUpdateResource(msg);
      break;

    case 'deleteResourceCompleted':
      handleDeleteResource(msg);
      break;

    case 'configureCompleted':
    case 'unregisterResourceCompleted':
    case 'enablePresenceCompleted':
    case 'disablePresenceCompleted':
    case 'sendResponseCompleted':
    case 'notifyCompleted':
    case 'cancelObservingCompleted':
    case 'createResourceCompleted':
      handleAsyncCallSuccess(msg);
      break;
    case 'asyncCallError':
      handleAsyncCallError(msg);
      break;
    default:
      DBG('Received unknown command');
      break;
  }
});

function handleRegisterResourceCompleted(msg) {

  DBG("handleRegisterResourceCompleted");
  DBG("msg.OicResourceInit=" + JSON.stringify(msg.OicResourceInit));
  var oicResource = new OicResource(JSON.stringify(msg.OicResourceInit));
  _addConstProperty(oicResource, 'id', msg.resourceId);
  _addConstProperty(oicResource, 'properties', JSON.stringify(msg.OicResourceInit.properties));
  g_async_calls[msg.asyncCallId].resolve(oicResource);
}

function handleEntityHandler(msg) {
  
  DBG("handleEntityHandler msg=" + JSON.stringify(msg));
 
  if (g_iotivity_device 
   && g_iotivity_device.server 
   && g_iotivity_device.server.onrequest) {

    var oicRequestEvent = new OicRequestEvent();
    _addConstProperty(oicRequestEvent, 'type', msg.type);
    _addConstProperty(oicRequestEvent, 'requestId', msg.requestId);
    _addConstProperty(oicRequestEvent, 'source', msg.source);
    _addConstProperty(oicRequestEvent, 'target', msg.target);

    if ((msg.type == 'create') || (msg.type == 'update')) {
      _addConstProperty(oicRequestEvent, 'properties', msg.res);
    }

    if (msg.type == 'update') {
      //TODO _addConstProperty(oicRequestEvent, 'updatedPropertyNames', msg.updatedPropertyNames);
      _addConstProperty(oicRequestEvent, 'updatedPropertyNames', msg.res);
    }

    DBG("server.onrequest: msg.type=" + msg.type);
    g_iotivity_device.server.onrequest(oicRequestEvent);
  }
}

function handleFoundResources(msg) {

  DBG("handleFoundResources msg=" + JSON.stringify(msg));

  // TODO: missing field from native FoundResourcesCallback!!!
  var oicResourceInit = new OicResourceInit({
    'url': msg.url,
    'deviceId': msg.resourceId,
    'connectionMode': msg.connectionMode,
    'resourceTypes': null,
    'interfaces': null,
    'discoverable': msg.discoverable,
    'observable': msg.observable,
    'parent': null,
    'children': null,
    'properties': null,
  });

  var oicResource = new OicResource(oicResourceInit);
  _addConstProperty(oicResource, 'id', msg.resourceId);
  //_addConstProperty(oicResource, 'properties', JSON.stringify(msg.OicResourceInit.properties));
  var oicResourceList = [];

  oicResourceList.push(oicResource);
  DBG("asyncCallId_onresourcesfound=" + asyncCallId_onresourcesfound);

  if (asyncCallId_onresourcesfound in g_async_calls) {
    DBG("g_async_calls[asyncCallId_onresourcesfound].resolve");
    g_async_calls[asyncCallId_onresourcesfound].resolve(oicResourceList);
    delete g_async_calls[asyncCallId_onresourcesfound];
    asyncCallId_onresourcesfound = 0;
  }
}

function handleOnObserve(msg) {
  
  DBG("handleOnObserve msg=" + JSON.stringify(msg));

  if (g_iotivity_device 
   && g_iotivity_device.client 
   && g_iotivity_device.client.onresourcechange) {

    // msg.type observe only
    var oicRequestEvent = new OicRequestEvent();
    _addConstProperty(oicRequestEvent, 'type', msg.type);
    _addConstProperty(oicRequestEvent, 'requestId', msg.requestId);
    _addConstProperty(oicRequestEvent, 'source', msg.source);
    _addConstProperty(oicRequestEvent, 'target', msg.target);

    _addConstProperty(oicRequestEvent, 'properties', msg.res);
    //TODO _addConstProperty(oicRequestEvent, 'updatedPropertyNames', msg.updatedPropertyNames);
    _addConstProperty(oicRequestEvent, 'updatedPropertyNames', msg.res);

    DBG("client.onresourcechange: msg.type=" + msg.type);
    g_iotivity_device.client.onresourcechange(oicRequestEvent);
  }
}

function handleRetrieveResource(msg) {
  
  DBG("handleRetrieveResource msg=" + JSON.stringify(msg));

  if (asyncCallId_onretrieveresource in g_async_calls) {

    if (msg.eCode == 0) {
      DBG("g_async_calls[asyncCallId_onresourcesfound].resolve");


  // TODO: missing field from native FoundResourcesCallback!!!
  var oicResourceInit = new OicResourceInit({
    'url': msg.url,
    'deviceId': msg.resourceId,
    'connectionMode': msg.connectionMode,
    'resourceTypes': null,
    'interfaces': null,
    'discoverable': msg.discoverable,
    'observable': msg.observable,
    'parent': null,
    'children': null,
    'properties': null,
  });

      var oicResource = new OicResource(oicResourceInit);
      _addConstProperty(oicResource, 'id', msg.resourceId);

      g_async_calls[asyncCallId_onretrieveresource].resolve(oicResource);
    }
    else {
      g_async_calls[asyncCallId_onretrieveresource].reject(Error('Command error'));
    }

    delete g_async_calls[asyncCallId_onretrieveresource];
    asyncCallId_onretrieveresource = 0;
  }
}

function handleUpdateResource(msg) {
  
  DBG("handleUpdateResource msg=" + JSON.stringify(msg));

  if (asyncCallId_onupdateresource in g_async_calls) {
    if (msg.eCode == 0) {
      DBG("g_async_calls[].resolve");
      g_async_calls[asyncCallId_onupdateresource].resolve();
    }
    else {
      g_async_calls[asyncCallId_onupdateresource].reject(Error('Command error'));
    }

    delete g_async_calls[asyncCallId_onupdateresource];
    asyncCallId_onupdateresource = 0;
  }
}

function handleDeleteResource(msg) {
  
  DBG("handleDeleteResource msg=" + JSON.stringify(msg));

  if (asyncCallId_onretrieveresource in g_async_calls) {
    if (msg.eCode == 0) {
      DBG("g_async_calls[].resolve");
      g_async_calls[asyncCallId_ondeleteresource].resolve();
    }
    else {
      g_async_calls[asyncCallId_ondeleteresource].reject(Error('Command error'));
    }

    delete g_async_calls[asyncCallId_ondeleteresource];
    asyncCallId_ondeleteresource = 0;
  }
}


function handleAsyncCallSuccess(msg) {
  if (msg.asyncCallId in g_async_calls) {
    g_async_calls[msg.asyncCallId].resolve();
    delete g_async_calls[msg.asyncCallId];
  }
}

function handleAsyncCallError(msg) {
  if (msg.asyncCallId in g_async_calls) {
    g_async_calls[msg.asyncCallId].reject(Error('Async operation failed'));
    delete g_async_calls[msg.asyncCallId];
  }
}

exports.handleMessageAsync = function(msg, callback) {
  iotivityListener = callback;
  extension.postMessage(msg);
};

exports.handleMessageSync = function(msg) {
  return extension.internal.sendSyncMessage(msg);
};






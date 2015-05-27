// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

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

///////////////////////////////////////////////////////////////////////////////
// Exports and main entry point for the MediaRenderer API
///////////////////////////////////////////////////////////////////////////////

var g_media_renderer_manager = new MediaRendererManager();
exports = g_media_renderer_manager;

var g_media_renderer_manager_listeners = {};
g_media_renderer_manager_listeners['rendererfound'] = [];
g_media_renderer_manager_listeners['rendererlost'] = [];
var g_media_renderers = [];

extension.setMessageListener(function(json) {
  var msg = JSON.parse(json);
  switch (msg.cmd) {
    case 'rendererFound':
      handleMediaRendererFound(msg);
      break;
    case 'rendererLost':
      handleMediaRendererLost(msg);
      break;
    case 'getRenderersCompleted':
      handleGetRenderersCompleted(msg);
      break;
    case 'playCompleted':
    case 'pauseCompleted':
    case 'stopCompleted':
    case 'nextCompleted':
    case 'previousCompleted':
    case 'setMuteCompleted':
    case 'setSpeedCompleted':
    case 'setVolumeCompleted':
    case 'gotoTrackCompleted':
      handleStatusChanged(msg);
    case 'openURICompleted':
    case 'prefetchURICompleted':
    case 'cancelCompleted':
      handleAsyncCallSuccess(msg);
      break;
    case 'asyncCallError':
      handleAsyncCallError(msg);
      break;
    default:
      console.error('[MediaRenderer]:' + "Unknown signal: '" + msg.cmd + "' from backend");
  }
});

function handleMediaRendererFound(msg) {
  var event = new CustomEvent('rendererfound');
  g_media_renderers[msg.renderer.id] = new MediaRenderer(msg.renderer);
  _addConstProperty(event, 'renderer', g_media_renderers[msg.renderer.id]);
  g_media_renderer_manager.dispatchEvent(event);
  if (g_media_renderer_manager.onrendererfound)
    g_media_renderer_manager.onrendererfound(event);
}

function handleMediaRendererLost(msg) {
  g_media_renderers[msg.rendererId] = null;
  var event = new CustomEvent('rendererlost');
  _addConstProperty(event, 'id', msg.rendererId);
  g_media_renderer_manager.dispatchEvent(event);
  if (g_media_renderer_manager.onrendererlost)
    g_media_renderer_manager.onrendererlost(event);
}

function handleStatusChanged(msg) {
  var event = new CustomEvent('statuschanged');
  _addConstPropertyFromObject(event, 'playbackStatus', msg.renderer.controller);
  _addConstPropertyFromObject(event, 'muted', msg.renderer.controller);
  _addConstPropertyFromObject(event, 'volume', msg.renderer.controller);
  _addConstPropertyFromObject(event, 'track', msg.renderer.controller);
  _addConstPropertyFromObject(event, 'speed', msg.renderer.controller);

  var renderer = g_media_renderers[msg.renderer.id];
  renderer.controller.dispatchEvent(event);
  if (renderer.controller.onstatuschanged) {
    renderer.controller.onstatuschanged(event);
  }
}

function handleAsyncCallSuccess(msg) {
  g_async_calls[msg.asyncCallId].resolve();
}

function handleAsyncCallError(msg) {
  g_async_calls[msg.asyncCallId].reject(Error('Async operation failed'));
}

function handleGetRenderersCompleted(msg) {
  g_async_calls[msg.asyncCallId].resolve(msg.renderers);
}

///////////////////////////////////////////////////////////////////////////////
// MediaRendererManager
///////////////////////////////////////////////////////////////////////////////

function MediaRendererManager() {
  this.onrendererfound = null;
  this.onrendererlost = null;
}

function isValidMediaRendererManagerEventType(type) {
  return (type === 'rendererlost' || type === 'rendererfound');
}

MediaRendererManager.prototype.addEventListener = function(type, callback) {
  if (callback != null && isValidMediaRendererManagerEventType(type))
    if (~~g_media_renderer_manager_listeners[type].indexOf(callback))
      g_media_renderer_manager_listeners[type].push(callback);
};

MediaRendererManager.prototype.removeEventListener = function(type, callback) {
  if (callback == null || !isValidMediaRendererManagerEventType(type))
    return;

  var index = g_media_renderer_manager_listeners[type].indexOf(callback);
  if (~index)
    g_media_renderer_manager_listeners[type].slice(index, 1);
};

MediaRendererManager.prototype.dispatchEvent = function(event) {
  var handled = true;

  if (typeof event !== 'object' || !isValidMediaRendererManagerEventType(event.type))
    return false;

  g_media_renderer_manager_listeners[event.type].forEach(function(callback) {
    var res = callback(event);
    if (!res && handled)
      handled = false;
  });

  return handled;
};

MediaRendererManager.prototype.scanNetwork = function() {
  var msg = {
    'cmd': 'scanNetwork'
  };
  extension.postMessage(JSON.stringify(msg));
};

function getRenderers() {
  var msg = {
    'cmd': 'getRenderers'
  };
  return createPromise(msg);
}

///////////////////////////////////////////////////////////////////////////////
// MediaRenderer
///////////////////////////////////////////////////////////////////////////////

function MediaRenderer(obj) {
  _addConstPropertyFromObject(this, 'id', obj);
  _addConstPropertyFromObject(this, 'friendlyName', obj);
  _addConstPropertyFromObject(this, 'manufacturer', obj);
  _addConstPropertyFromObject(this, 'manufacturerURL', obj);
  _addConstPropertyFromObject(this, 'modelDescription', obj);
  _addConstPropertyFromObject(this, 'modelName', obj);
  _addConstPropertyFromObject(this, 'modelNumber', obj);
  _addConstPropertyFromObject(this, 'serialNumber', obj);
  _addConstPropertyFromObject(this, 'UDN', obj);
  _addConstPropertyFromObject(this, 'presentationURL', obj);
  _addConstPropertyFromObject(this, 'iconURL', obj);
  _addConstPropertyFromObject(this, 'deviceType', obj);
  _addConstPropertyFromObject(this, 'protocolInfo', obj);
  _addConstProperty(this, 'controller', new MediaController(obj.controller));
}

MediaRenderer.prototype.openURI = function(mediaURI, metaData) {
  var msg = {
    'cmd': 'openURI',
    'rendererId': this.id,
    'mediaURI': mediaURI,
    'metaData': metaData
  };
  return createPromise(msg);
};

MediaRenderer.prototype.prefetchURI = function(mediaURI, metaData) {
  var msg = {
    'cmd': 'prefetchURI',
    'rendererId': this.id,
    'mediaURI': mediaURI,
    'metaData': metaData
  };
  return createPromise(msg);
};

MediaRenderer.prototype.cancel = function() {
  var msg = {
    'cmd': 'cancel',
    'rendererId': this.id
  };
  return createPromise(msg);
};

///////////////////////////////////////////////////////////////////////////////
// MediaController
///////////////////////////////////////////////////////////////////////////////

function isValidMediaControllerEventType(type) {
  return (type === 'statuschanged');
}

MediaController.prototype.addEventListener = function(type, callback) {
  if (callback != null &&
      isValidMediaControllerEventType(type) &&
      ~~this.media_controller_listeners[type].indexOf(callback))
    this.media_controller_listeners[type].push(callback);
};

MediaController.prototype.removeEventListener = function(type, callback) {
  if (callback == null || !isValidMediaControllerEventType(type))
    return;

  var index = this.media_controller_listeners[type].indexOf(callback);
  if (~index)
    this.media_controller_listeners[type].slice(index, 1);
};

MediaController.prototype.dispatchEvent = function(event) {
  var handled = true;

  if (typeof event !== 'object' || !isValidMediaControllerEventType(event.type))
    return false;

  this.media_controller_listeners[event.type].forEach(function(callback) {
    var res = callback(event);
    if (!res)
      handled = false;
  });

  return handled;
};

function MediaController(obj) {
  _addConstPropertyFromObject(this, 'id', obj);
  _addConstPropertyFromObject(this, 'playbackStatus', obj);
  _addConstPropertyFromObject(this, 'muted', obj);
  _addConstPropertyFromObject(this, 'volume', obj);
  _addConstPropertyFromObject(this, 'track', obj);
  _addConstPropertyFromObject(this, 'speed', obj);
  _addConstPropertyFromObject(this, 'playSpeeds', obj);
  this.onstatuschanged = null;
  this.media_controller_listeners = {};
  this.media_controller_listeners['statuschanged'] = [];
}

MediaController.prototype.play = function() {
  var msg = {
    'cmd': 'play',
    'rendererId': this.id
  };
  return createPromise(msg);
};

MediaController.prototype.pause = function() {
  var msg = {
    'cmd': 'pause',
    'rendererId': this.id
  };
  return createPromise(msg);
};

MediaController.prototype.stop = function() {
  var msg = {
    'cmd': 'stop',
    'rendererId': this.id
  };
  return createPromise(msg);
};

MediaController.prototype.next = function() {
  var msg = {
    'cmd': 'next',
    'rendererId': this.id
  };
  return createPromise(msg);
};

MediaController.prototype.previous = function() {
  var msg = {
    'cmd': 'previous',
    'rendererId': this.id
  };
  return createPromise(msg);
};

MediaController.prototype.mute = function(mute) {
  var msg = {
    'cmd': 'setMute',
    'rendererId': this.id,
    'mute': mute
  };
  return createPromise(msg);
};

MediaController.prototype.setSpeed = function(speed) {
  var msg = {
    'cmd': 'setSpeed',
    'rendererId': this.id,
    'speed': speed
  };
  return createPromise(msg);
};

MediaController.prototype.setVolume = function(volume) {
  var msg = {
    'cmd': 'setVolume',
    'rendererId': this.id,
    'volume': volume
  };
  return createPromise(msg);
};

MediaController.prototype.gotoTrack = function(track) {
  var msg = {
    'cmd': 'gotoTrack',
    'rendererId': this.id,
    'track': track
  };
  return createPromise(msg);
};

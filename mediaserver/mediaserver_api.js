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

function derive(child, parent) {
  child.prototype = Object.create(parent.prototype);
  child.prototype.constructor = child;
  _addConstructorProperty(child.prototype, child);
}

///////////////////////////////////////////////////////////////////////////////
// Exports and main entry point for the MediaServer API
///////////////////////////////////////////////////////////////////////////////

var g_media_server_manager = new MediaServerManager();
exports = g_media_server_manager;

var g_media_server_manager_listeners = {};
g_media_server_manager_listeners['serverfound'] = [];
g_media_server_manager_listeners['serverlost'] = [];

extension.setMessageListener(function(json) {
  var msg = JSON.parse(json);
  switch (msg.cmd) {
    case 'serverFound':
      handleMediaServerFound(msg);
      break;
    case 'serverLost':
      handleMediaServerLost(msg);
      break;
    case 'getServersCompleted':
      handleGetServersCompleted(msg);
      break;
    case 'browseCompleted':
    case 'findCompleted':
      handleBrowseCompleted(msg);
      break;
    case 'createFolderCompleted':
    case 'uploadCompleted':
    case 'cancelCompleted':
    case 'uploadToContainerCompleted':
    case 'createFolderInContainerCompleted':
      handleAsyncCallSuccess(msg);
      break;
    case 'asyncCallError':
      handleAsyncCallError(msg);
      break;
  }
});

function handleMediaServerFound(msg) {
  var event = new CustomEvent('serverfound');
  _addConstProperty(event, 'server', new MediaServer(msg.server));
  g_media_server_manager.dispatchEvent(event);
  if (g_media_server_manager.onserverfound)
    g_media_server_manager.onserverfound(event);
}

function handleMediaServerLost(msg) {
  var event = new CustomEvent('serverlost');
  _addConstProperty(event, 'id', msg.lostServerId);
  g_media_server_manager.dispatchEvent(event);
  if (g_media_server_manager.onserverlost)
    g_media_server_manager.onserverlost(event);
}

function handleBrowseCompleted(msg) {
  var mediaObjects = convertToMediaObjects(msg.mediaObjects);
  g_async_calls[msg.asyncCallId].resolve(mediaObjects);
}

function handleAsyncCallSuccess(msg) {
  g_async_calls[msg.asyncCallId].resolve();
}

function handleAsyncCallError(msg) {
  g_async_calls[msg.asyncCallId].reject(Error('Async operation failed'));
}

function handleGetServersCompleted(msg) {
  g_async_calls[msg.asyncCallId].resolve(msg.servers);
}

function convertToMediaObjects(objArray) {
  var objects = [];
  objArray.forEach(function(element) {
    this.push(convertToMediaObject(element));
  }, objects);
  return objects;
}

function convertToMediaObject(obj) {
  if (obj.type === 'container')
    return new MediaContainer(obj);
  else
    return new MediaItem(obj);
}

///////////////////////////////////////////////////////////////////////////////
// MediaServerManager
///////////////////////////////////////////////////////////////////////////////

function MediaServerManager() {
  this.onserverfound = null;
  this.onserverlost = null;
}

function isValidType(type) {
  return (type === 'serverlost' || type === 'serverfound');
}

MediaServerManager.prototype.addEventListener = function(type, callback) {
  if (callback != null && isValidType(type))
    if (~~g_media_server_manager_listeners[type].indexOf(callback))
      g_media_server_manager_listeners[type].push(callback);
};

MediaServerManager.prototype.removeEventListener = function(type, callback) {
  if (callback != null && isValidType(type)) {
    var index = g_media_server_manager_listeners[type].indexOf(callback);
    if (~index)
      g_media_server_manager_listeners[type].slice(index, 1);
  }
};

MediaServerManager.prototype.dispatchEvent = function(event) {
  var handled = true;

  if (typeof event !== 'object' || !isValidType(event.type))
    return false;

  g_media_server_manager_listeners[event.type].forEach(function(callback) {
    var res = callback(event);
    if (!res && handled)
      handled = false;
  });

  return handled;
};

MediaServerManager.prototype.scanNetwork = function() {
  var msg = {
    'cmd': 'scanNetwork'
  };
  extension.postMessage(JSON.stringify(msg));
};

function getServers() {
  var msg = {
    'cmd': 'getServers'
  };
  return createPromise(msg);
}

MediaServerManager.prototype.find = function(query, sortOptions) {
  return new Promise(function(resolve, reject) {
    getServers().then(
        function onServersFound(servers) {
          var promises = [];
          servers.forEach(function(server) {
            var mediaServer = new MediaServer(server);
            promises.push(mediaServer.find(server.root.id, query,
                          createSortString(sortOptions), 0, 0));
          });

          Promise.all(promises).then(function(results) {
            var mediaItems = [];
            results.forEach(function(result) {
              mediaItems = mediaItems.concat(result);
            });
            resolve(mediaItems);
          }, reject);
        }, reject);
  });
};

///////////////////////////////////////////////////////////////////////////////
// MediaServer
///////////////////////////////////////////////////////////////////////////////

function MediaServer(obj) {
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
  _addConstProperty(this, 'root', new MediaContainer(obj.root));
  _addConstPropertyFromObject(this, 'canCreateContainer', obj);
  _addConstPropertyFromObject(this, 'canUpload', obj);
  _addConstPropertyFromObject(this, 'searchAttrs', obj);
  _addConstPropertyFromObject(this, 'sortAttrs', obj);
  _addConstPropertyFromObject(this, 'searchAttrs', obj);
  this.oncontainerchanged = null;
}

MediaServer.prototype.upload = function(path) {
  var msg = {
    'cmd': 'upload',
    'serverId': this.root.id,
    'title': path.replace(/^.*[\\\/]/, ''),
    'path': path
  };
  return createPromise(msg);
};

MediaServer.prototype.createFolder = function(folderName) {
  var msg = {
    'cmd': 'createFolder',
    'serverId': this.root.id,
    'folderName': folderName
  };
  return createPromise(msg);
};

function createSortString(str) {
  var sortString;
  if (str.length) {
    sortString = str.replace('ASC', '+');
    sortString = sortString.replace('DESC', '-');
  }
  return sortString;
}

MediaServer.prototype.browse = function(containerId, sortMode, count, offset) {
  var msg = {
    'cmd': 'browse',
    'serverId': this.root.id,
    'containerId': containerId,
    'sortMode': createSortString(sortMode),
    'count': count,
    'offset': offset
  };
  return createPromise(msg);
};

MediaServer.prototype.find =
    function(containerId, searchFilter, sortMode, count, offset) {
  var msg = {
    'cmd': 'find',
    'serverId': this.root.id,
    'containerId': containerId,
    'searchFilter': searchFilter,
    'sortMode': createSortString(sortMode),
    'count': count,
    'offset': offset
  };
  return createPromise(msg);
};

MediaServer.prototype.cancel = function() {
  var msg = {
    'cmd': 'cancel',
    'serverId': this.root.id
  };
  return createPromise(msg);
};

///////////////////////////////////////////////////////////////////////////////
// MediaObject, MediaItem and MediaContainer
///////////////////////////////////////////////////////////////////////////////

function MediaObject(obj) {
  _addConstPropertyFromObject(this, 'id', obj);
  _addConstPropertyFromObject(this, 'type', obj);
  _addConstPropertyFromObject(this, 'title', obj);
}

function MediaContainer(obj) {
  MediaObject.call(this, obj);
  _addConstPropertyFromObject(this, 'childCount', obj);
  _addConstPropertyFromObject(this, 'rootContainerId', obj);
}
derive(MediaContainer, MediaObject);

MediaContainer.prototype.upload = function(title, path) {
  var msg = {
    'cmd': 'uploadToContainer',
    'containerId': this.id,
    'rootContainerId': this.rootContainerId,
    'title': title,
    'path': path
  };
  return createPromise(msg);
};

MediaContainer.prototype.createFolder = function(title) {
  var msg = {
    'cmd': 'createFolderInContainer',
    'containerId': this.id,
    'rootContainerId': this.rootContainerId,
    'title': title
  };
  return createPromise(msg);
};

function MediaItem(obj) {
  // Strip .* from the type
  var index = obj.type.indexOf('.');
  if (index !== -1)
    obj.type = obj.type.substr(0, index);

  MediaObject.call(this, obj);
  _addConstPropertyFromObject(this, 'mimeType', obj);
  _addConstProperty(this, 'sourceUri', obj.sourceUri);
  _addConstProperty(this, 'createDate', obj.createDate);
  _addConstProperty(this, 'fileSize', obj.fileSize);
  _addConstProperty(this, 'width', obj.width);
  _addConstProperty(this, 'height', obj.height);
  _addConstProperty(this, 'duration', obj.duration);
  _addConstProperty(this, 'audioSampleRate', obj.audioSampleRate);
  _addConstProperty(this, 'collection', obj.collection);
  _addConstProperty(this, 'author', obj.author);
  _addConstProperty(this, 'category', obj.category);
  _addConstProperty(this, 'trackNumber', obj.trackNumber);
}
derive(MediaItem, MediaObject);

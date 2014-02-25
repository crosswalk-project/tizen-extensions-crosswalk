// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var _callbacks = {};
var _nextReplyId = 0;

function getNextReplyId() {
  return _nextReplyId++;
};

function postMessage(msg, callback) {
  var replyId = getNextReplyId();
  _callbacks[replyId] = callback;
  msg.replyId = replyId;
  extension.postMessage(JSON.stringify(msg));
};

function sendSyncMessage(msg) {
  return extension.internal.sendSyncMessage(JSON.stringify(msg));
};

extension.setMessageListener(function(msg) {
  var m = JSON.parse(msg);
  var replyId = m.replyId;
  var callback = _callbacks[replyId];
  
  if (typeof(callback) === 'function') {
    callback(m);
    delete m.replyId;
    delete _callbacks[replyId];
  } else {
    console.log('Invalid replyId from Tizen Content API: ' + replyId);
  }
});

function Folder(uri, id, type, title) {
  Object.defineProperties(this, {
    'directoryURI': { writable: false, value: uri, enumerable: true },
    'id': { writable: false, value: id, enumerable: true },
    'storageType': { writable: false, value: type, enumerable: true },
    'title': { writable: false, value: title, enumerable: true }
  });
}

function Content(id, name, type, mimeType, title, contentURI, thumnailURIs, releaseDate, modifiedDate, size, description, rating) {
  Object.defineProperties(this, {
    'id': { writable: false, value: id, enumerable: true },
    'name': { writable: false, value: name, enumerable: true },
    'type': { writable: false, value: type, enumerable: true },
    'mimeType': { writable: false, value: mimeType, enumerable: true },
    'title': { writable: false, value: title, enumerable: true },
    'contentURI': { writable: false, value: contentURI, enumerable: true },
    'thumnailURIs': { writable: false, value: thumnailURIs, enumerable: true },
    'releaseDate': { writable: false, value: releaseDate, enumerable: true },
    'modifiedDate': { writable: false, value: modifiedDate, enumerable: true },
    'size': { writable: false, value: size, enumerable: true },
    'description': { writable: false, value: description, enumerable: true },
    'rating': { writable: false, value: rating, enumerable: true },
  });
}

function ContentManager() {
}

ContentManager.prototype.update = function(content) {
}

ContentManager.prototype.updateBatch = function(content, onsuccess, onerror) {
}

ContentManager.prototype.getDirectories = function(onsuccess, onerror) {
  postMessage({
      cmd: 'ContentManager.getDirectories',
    }, function(result) {
      if (result.isError) {
        if (onerror)
          onerror(new tizen.WebAPIError(result.errorCode));
      } else if (onsuccess) {
        var folders = [];

        for (var i = 0; i < result.value.length; i++) {
          var folder = result.value[i];
          var jsonFolder = new Folder(folder.directoryURI, folder.id, folder.storageType, folder.title);
          folders.push(jsonFolder)
        }
        onsuccess(folders);
      }
  });
}

ContentManager.prototype.find = function(onsuccess, onerror, directoryId, filter, sortMode, count, offset) {
  postMessage({
      cmd: 'ContentManager.find',
      directoryId: directoryId,
      filter: filter,
      sortMode: sortMode,
      count: count,
      offset: offset,
    }, function(result) {
      if (result.isError) {
        if (onerror)
          onerror(new tizen.WebAPIError(result.errorCode));
      } else if (onsuccess) {
        var contents = [];
        for (var i = 0; i < result.value.length; i++) {
          var content = result.value[i];
          var jsonContent = new Content(content.id, 
                  content.name, 
                  content.type, 
                  content.mimeType, 
                  content.title, 
                  content.contentURI, 
                  content.thumnailURIs, 
                  content.releaseDate, 
                  content.modifiedDate, 
                  content.size, 
                  content.description, 
                  content.rating);
          contents.push(jsonContent);
        }     
        onsuccess(contents);
      }
  });
}

ContentManager.prototype.scanFile = function(contentURI, onsuccess, onerror) {
}

ContentManager.prototype.setChangeListener = function(onchange) {
}

ContentManager.prototype.unsetChangeLIstener = function() {
}

exports = new ContentManager();

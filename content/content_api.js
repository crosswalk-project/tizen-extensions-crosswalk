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

function ContentAudio(obj, album, genres, artists, composer, copyright, bitrate, trackNumber, duration) {
  Object.defineProperties(obj, {
    'album': { writable: false, value: album, enumerable: true },
    'genres': { writable: false, value: genres, enumerable: true },
    'artists': { writable: false, value: artists, enumerable: true },
    'composer': { writable: false, value: composer, enumerable: true },
    'copyright': { writable: false, value: copyright, enumerable: true },
    'bitrate': { writable: false, value: bitrate, enumerable: true },
    'trackNumber': { writable: false, value: trackNumber, enumerable: true },
    'duration': { writable: false, value: duration, enumerable: true },
  });
}

function ContentImage(obj, width, height, orientation) {
  Object.defineProperties(obj, {
    'width': { writable: false, value: width, enumerable: true },
    'height': { writable: false, value: height, enumerable: true },
    'orientation': { writable: false, value: orientation, enumerable: true },
  });
}

function ContentVideo(obj, album, artists, duration, width, height) {
  Object.defineProperties(obj, {
    'album': { writable: false, value: album, enumerable: true },
    'artists': { writable: false, value: artists, enumerable: true },
    'duration': { writable: false, value: duration, enumerable: true },
    'width': { writable: false, value: width, enumerable: true },
    'height': { writable: false, value: height, enumerable: true },
  });
}

function ContentManager() {
}

ContentManager.prototype.update = function(content) {
  if (!xwalk.utils.validateArguments('o', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }
}

ContentManager.prototype.updateBatch = function(content, onsuccess, onerror) {
  if (!xwalk.utils.validateArguments('o?ff', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }
}

ContentManager.prototype.getDirectories = function(onsuccess, onerror) {
  if (!xwalk.utils.validateArguments('f?f', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

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
          var jsonFolder = new Folder(folder.directoryURI,
              folder.id,
              folder.storageType,
              folder.title);
          folders.push(jsonFolder)
        }
        onsuccess(folders);
      }
  });
}

ContentManager.prototype.find = function(onsuccess, onerror, directoryId, filter, sortMode, count, offset) {
  if (!xwalk.utils.validateArguments('f?fsoonn', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

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

          if (content.type == "AUDIO") {
            ContentAudio(jsonContent,
                content.album,
                content.genres,
                content.artists,
                content.composer,
                content.copyright,
                content.bitrate,
                content.trackNumber,
                content.duration);
          } else if (content.type == "IMAGE") {
            ContentImage(jsonContent,
                content.width,
                content.height,
                content.orientation);
          } else if (content.type == "VIDEO") {
            ContentImage(jsonContent,
                content.album,
                content.artists,
                content.duration,
                content.width,
                content.height);
          }
          contents.push(jsonContent);
        }     
        onsuccess(contents);
      }
  });
}

ContentManager.prototype.scanFile = function(contentURI, onsuccess, onerror) {
  if (!xwalk.utils.validateArguments('s?ff', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }
  postMessage({
      cmd: 'ContentManager.scanFile',
      contentURI: contentURI
    }, function(result) {
      if (result.isError) {
        if (onerror)
          onerror(new tizen.WebAPIError(result.errorCode));
      } else if (onsuccess) {
        onsuccess(contentURI);
      }
  });
}

ContentManager.prototype.setChangeListener = function(onchange) {
  if (!xwalk.utils.validateArguments('f', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }
}

ContentManager.prototype.unsetChangeLIstener = function() {
}

exports = new ContentManager();

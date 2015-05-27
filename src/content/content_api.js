// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var _callbacks = {};
var _nextReplyId = 1; // 0 is reserved for events

function getNextReplyId() {
  return _nextReplyId++;
}

function postMessage(msg, callback) {
  var replyId = getNextReplyId();
  _callbacks[replyId] = callback;
  msg.replyId = replyId;
  extension.postMessage(JSON.stringify(msg));
}

function sendSyncMessage(msg) {
  return extension.internal.sendSyncMessage(JSON.stringify(msg));
}

extension.setMessageListener(function(msg) {
  var m = JSON.parse(msg);
  var replyId = m.replyId;
  var callback = _callbacks[replyId];

  if (replyId == 0) { // replyId zero is for events
    if (exports.changeListener != null) {
      if (m.eventType == 'INSERT')
        exports.changeListener.oncontentadded(m.value);
      else if (m.eventType == 'DELETE')
        exports.changeListener.oncontentremoved(m.value.id);
      else if (m.eventType == 'UPDATE')
        exports.changeListener.oncontentupdated(m.value);
    }
  } else if (typeof(callback) === 'function') {
    callback(m);
    delete m.replyId;
    delete _callbacks[replyId];
  } else {
    console.log('Invalid replyId from Tizen Content API: ' + replyId);
  }
});

function parseDate(date) {
  if (typeof(date) !== 'string' || date === '')
    return null;
  return new Date(date);
}

function Folder(uri, id, type, title) {
  Object.defineProperties(this, {
    'directoryURI': { writable: false, value: uri, enumerable: true },
    'id': { writable: false, value: id, enumerable: true },
    'storageType': { writable: false, value: type, enumerable: true },
    'title': { writable: false, value: title, enumerable: true }
  });
}

function Content(editableAttributes, id, name, type, mimeType, title, contentURI, thumnailURIs,
    releaseDate, modifiedDate, size, description, rating) {
  var rating_ = rating, name_ = name;
  Object.defineProperties(this, {
    'editableAttributes': { writable: false, value: editableAttributes, enumerable: true },
    'id': { writable: false, value: id, enumerable: true },
    'name': {
      enumerable: true,
      set: function(v) { if (v != null) name_ = v},
      get: function() { return name_; }
    },
    'type': { writable: false, value: type, enumerable: true },
    'mimeType': { writable: false, value: mimeType, enumerable: true },
    'title': { writable: false, value: title, enumerable: true },
    'contentURI': { writable: false, value: contentURI, enumerable: true },
    'thumnailURIs': { writable: false, value: thumnailURIs, enumerable: true },
    'releaseDate': { writable: false, value: parseDate(releaseDate), enumerable: true },
    'modifiedDate': { writable: false, value: parseDate(modifiedDate), enumerable: true },
    'size': { writable: false, value: size, enumerable: true },
    'description': { writable: true, value: description, enumerable: true },
    'rating': {
      enumerable: true,
      set: function(v) { if (v != null && v >= 0 && v <= 10) rating_ = v; },
      get: function() { return rating_; }
    }
  });
}

function AudioContent(obj, album, genres, artists, composer, copyright,
    bitrate, trackNumber, duration) {
  Object.defineProperties(obj, {
    'album': { writable: false, value: album, enumerable: true },
    'genres': { writable: false, value: genres, enumerable: true },
    'artists': { writable: false, value: artists, enumerable: true },
    'composer': { writable: false, value: composer, enumerable: true },
    'copyright': { writable: false, value: copyright, enumerable: true },
    'bitrate': { writable: false, value: bitrate, enumerable: true },
    'trackNumber': { writable: false, value: trackNumber, enumerable: true },
    'duration': { writable: false, value: duration, enumerable: true }
  });
}

function ImageContent(obj, geolocation, width, height, orientation) {
  var orientation_ = orientation;
  Object.defineProperties(obj, {
    'geolocation': { writable: true, value: geolocation, enumerable: true },
    'width': { writable: false, value: width, enumerable: true },
    'height': { writable: false, value: height, enumerable: true },
    'orientation': {
      enumerable: true,
      set: function(v) { if (v != null) orientation_ = v; },
      get: function() { return orientation_; }
    }
  });
}

function VideoContent(obj, geolocation, album, artists, duration, width, height) {
  Object.defineProperties(obj, {
    'geolocation': { writable: true, value: geolocation, enumerable: true },
    'album': { writable: false, value: album, enumerable: true },
    'artists': { writable: false, value: artists, enumerable: true },
    'duration': { writable: false, value: duration, enumerable: true },
    'width': { writable: false, value: width, enumerable: true },
    'height': { writable: false, value: height, enumerable: true }
  });
}

function ContentManager() {
  this.changeListener = null;
}

ContentManager.prototype.update = function(content) {
  if (!xwalk.utils.validateArguments('o', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }
  sendSyncMessage({
    cmd: 'ContentManager.update',
    content: content
  });
};

ContentManager.prototype.updateBatch = function(content, onsuccess, onerror) {
  if (!xwalk.utils.validateArguments('o?ff', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  postMessage({
    cmd: 'ContentManager.updateBatch',
    content: content
  }, function(result) {
    if (result.isError) {
      if (onerror)
        onerror(new tizen.WebAPIError(result.errorCode));
    } else if (onsuccess) {
      onsuccess();
    }
  });
};

ContentManager.prototype.getDirectories = function(onsuccess, onerror) {
  if (!xwalk.utils.validateArguments('f?f', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  postMessage({
    cmd: 'ContentManager.getDirectories'
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
        folders.push(jsonFolder);
      }
      onsuccess(folders);
    }
  });
};

ContentManager.prototype.find = function(onsuccess, onerror, directoryId,
    filter, sortMode, count, offset) {
  if (!xwalk.utils.validateArguments('f?fsoonn', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  postMessage({
    cmd: 'ContentManager.find',
    directoryId: directoryId,
    filter: filter,
    sortMode: sortMode,
    count: count,
    offset: offset
  }, function(result) {
    if (result.isError) {
      if (onerror)
        onerror(new tizen.WebAPIError(result.errorCode));
    } else if (onsuccess) {
      var contents = [];
      for (var i = 0; i < result.value.length; i++) {
        var content = result.value[i];
        var jsonContent = new Content(content.editableAttributes,
            content.id,
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

        if (content.type == 'AUDIO') {
          AudioContent(jsonContent,
              content.album,
              content.genres,
              content.artists,
              content.composer,
              content.copyright,
              content.bitrate,
              content.trackNumber,
              content.duration);
        } else if (content.type == 'IMAGE') {
          var geolocation = new tizen.SimpleCoordinates(content.latitude, content.longitude);
          ImageContent(jsonContent,
              geolocation,
              content.width,
              content.height,
              content.orientation);
        } else if (content.type == 'VIDEO') {
          var geolocation = new tizen.SimpleCoordinates(content.latitude, content.longitude);
          VideoContent(jsonContent,
              geolocation,
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
};

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
};

ContentManager.prototype.setChangeListener = function(listener) {
  if (!xwalk.utils.validateArguments('o', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  if (!xwalk.utils.validateObject(listener, 'fff',
      ['oncontentadded', 'oncontentupdated', 'oncontentremoved'])) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  this.changeListener = listener;
  sendSyncMessage({cmd: 'ContentManager.setChangeListener'});
};

ContentManager.prototype.unsetChangeListener = function() {
  this.changeListener = null;
  sendSyncMessage({cmd: 'ContentManager.unsetChangeListener'});
};

exports = new ContentManager();

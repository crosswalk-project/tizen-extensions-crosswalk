// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var _callbacks = {};
var _next_reply_id = 0;

var _listeners = [];
var _next_listener_id = 0;

var getNextReplyId = function() {
  return _next_reply_id++;
};

var encodings = {'UTF-8' : 1, 'ISO8859-1' : 1}; // gyp injection here

var postMessage = function(msg, callback) {
  var reply_id = getNextReplyId();
  _callbacks[reply_id] = callback;
  msg.reply_id = reply_id;
  extension.postMessage(JSON.stringify(msg));
};

extension.setMessageListener(function(json) {
  var msg = JSON.parse(json);
  if (msg.cmd === 'storageChanged') {
    handleStorageChanged(msg);
  } else {
    var reply_id = msg.reply_id;
    var callback = _callbacks[reply_id];
    if (typeof(callback) === 'function') {
      callback(msg);
      delete msg.reply_id;
      delete _callbacks[reply_id];
    } else {
      console.log('Invalid reply_id from Tizen Filesystem: ' + reply_id);
    }
  }
});

var sendSyncMessage = function(msg, args) {
  args = args || {};
  args.cmd = msg;
  return JSON.parse(extension.internal.sendSyncMessage(JSON.stringify(args)));
};

var FileSystemStorage = function(label, type, state) {
  Object.defineProperties(this, {
    'label': { writable: false, value: label, enumerable: true },
    'type': { writable: false, value: type, enumerable: true },
    'state': { writable: false, value: state, enumerable: true }
  });
};

var getFileParent = function(childPath) {
  if (childPath.search('/') < 0)
    return null;

  var parentPath = childPath.substr(0, childPath.lastIndexOf('/'));
  return new File(parentPath, getFileParent(parentPath));
};

function is_string(value) { return typeof(value) === 'string' || value instanceof String; }
function is_integer(value) { return isFinite(value) && !isNaN(parseInt(value)); }
function get_valid_mode(mode) {
  if (mode === null)
    return 'rw';
  else if (mode === 'a' || mode === 'w' || mode === 'r' || mode === 'rw')
    return mode;
  else
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
}

function FileSystemManager() {
  Object.defineProperty(this, 'maxPathLength', {
    get: function() {
      var message = sendSyncMessage('FileSystemManagerGetMaxPathLength');
      if (message.isError)
        return 4096;
      return message.value;
    },
    enumerable: true
  });
}

FileSystemManager.prototype.resolve = function(location, onsuccess,
    onerror, mode) {
  if (!(onsuccess instanceof Function))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  if (onerror !== null && !(onerror instanceof Function) &&
      arguments.length > 2)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  if (arguments.length < 4)
    mode = null;
  mode = get_valid_mode(mode);

  postMessage({
    cmd: 'FileSystemManagerResolve',
    location: location,
    mode: mode
  }, function(result) {
    if (result.isError)
      onerror(new tizen.WebAPIException(result.errorCode));
    else
      onsuccess(new File(result.fullPath, getFileParent(result.fullPath)));
  });
};

FileSystemManager.prototype.getStorage = function(label, onsuccess, onerror) {
  if (!(onsuccess instanceof Function))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  if (onerror !== null && !(onerror instanceof Function) &&
      arguments.length > 2)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  postMessage({
    cmd: 'FileSystemManagerGetStorage',
    label: label
  }, function(result) {
    if (result.isError)
      onerror(new tizen.WebAPIError(result.errorCode));
    else
      onsuccess(new FileSystemStorage(result.label, result.type, result.state));
  });
};

FileSystemManager.prototype.listStorages = function(onsuccess, onerror) {
  if (!(onsuccess instanceof Function))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  if (onerror !== null && !(onerror instanceof Function) &&
      arguments.length > 1)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  postMessage({
    cmd: 'FileSystemManagerListStorages'
  }, function(result) {
    if (result.isError)
      onerror(new tizen.WebAPIError(result.errorCode));
    else {
      var storages = [];
      for (var i = 0; i < result.value.length; i++) {
        var storage = result.value[i];
        storages.push(new FileSystemStorage(storage.label, storage.type, storage.state));
      }
      onsuccess(storages);
    }
  });
};

function handleStorageChanged(msg) {
  var storage = msg.storage;
  _listeners.forEach(function(id) {
    _listeners[id](new FileSystemStorage(storage.label, storage.type, storage.state));
  });
}

FileSystemManager.prototype.addStorageStateChangeListener = function(onsuccess, onerror) {
  if (!(onsuccess instanceof Function))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  if (onerror !== null && !(onerror instanceof Function) &&
      arguments.length > 1)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  _listeners[_next_listener_id] = onsuccess;
  return _next_listener_id++;
};

FileSystemManager.prototype.removeStorageStateChangeListener = function(watchId) {
  if (watchId !== undefined && typeof(watchId) !== 'number')
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);


  if (_listeners[watchId])
    _listeners[watchId] = null;
  else
    throw new tizen.WebAPIException(tizen.WebAPIException.NOT_FOUND_ERR);
};

function FileStream(streamID) {
  this.streamID = streamID;

  function fs_stat(streamID) {
    var result = sendSyncMessage('FileStreamStat', { streamID: streamID });
    if (result.isError)
      return result;
    return result.value;
  }

  var getStreamID = function() {
    return streamID;
  };

  var isEof = function() {
    var status = fs_stat(streamID);
    if (status.isError)
      return true;
    return status.eof;
  };
  var getPosition = function() {
    var status = fs_stat(streamID);
    if (status.isError)
      return -1;
    return status.position;
  };
  var setPosition = function(position) {
    if (!(is_integer(position)))
      return;

    var result = sendSyncMessage('FileStreamSetPosition',
                                 { streamID: this.streamID,
                                   position: position });
    if (result.isError)
      throw new tizen.WebAPIException(result.errorCode);
  };
  var getBytesAvailable = function() {
    var status = fs_stat(streamID);
    if (status.isError)
      return -1;
    return status.bytesAvailable;
  };

  Object.defineProperties(this, {
    'streamID': { get: getStreamID, enumerable: false },
    'position': { get: getPosition, set: setPosition, enumerable: true },
    'eof': { get: isEof, enumerable: true },
    'bytesAvailable': { get: getBytesAvailable, enumerable: true }
  });
}

FileStream.prototype.close = function() {
  sendSyncMessage('FileStreamClose', {
    streamID: this.streamID
  });
};

FileStream.prototype.read = function(charCount) {
  if (!(is_integer(charCount)))
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);

  var result = sendSyncMessage('FileStreamRead', {
    streamID: this.streamID,
    type: 'Default',
    count: charCount
  });
  if (result.isError)
    throw new tizen.WebAPIException(result.errorCode);
  else
    return result.value;
};

FileStream.prototype.readBytes = function(byteCount) {
  if (!(is_integer(byteCount)))
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);

  var result = sendSyncMessage('FileStreamRead', {
    streamID: this.streamID,
    type: 'Bytes',
    count: byteCount
  });
  if (result.isError)
    throw new tizen.WebAPIException(result.errorCode);
  else
    return result.value;
};

FileStream.prototype.readBase64 = function(byteCount) {
  if (!(is_integer(byteCount)))
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);

  var result = sendSyncMessage('FileStreamRead', {
    streamID: this.streamID,
    type: 'Base64',
    count: byteCount
  });
  if (result.isError)
    throw new tizen.WebAPIException(result.errorCode);
  else
    return result.value;
};

FileStream.prototype.write = function(stringData) {
  if (!(is_string(stringData)))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var result = sendSyncMessage('FileStreamWrite', {
    streamID: this.streamID,
    type: 'Default',
    data: stringData
  });
  if (result.isError)
    throw new tizen.WebAPIException(result.errorCode);
};

FileStream.prototype.writeBytes = function(byteData) {
  if (!Array.isArray(byteData))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var result = sendSyncMessage('FileStreamWrite', {
    streamID: this.streamID,
    type: 'Bytes',
    data: byteData
  });
  if (result.isError)
    throw new tizen.WebAPIException(result.errorCode);
};

FileStream.prototype.writeBase64 = function(base64Data) {
  if (!(is_string(base64Data)))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var result = sendSyncMessage('FileStreamWrite', {
    streamID: this.streamID,
    type: 'Base64',
    data: base64Data
  });
  if (result.isError)
    throw new tizen.WebAPIException(result.errorCode);
};

function File(fullPath, parent) {
  this.fullPath = fullPath;
  this.parent = parent;

  var stat_cached = undefined;
  var stat_last_time = undefined;

  function stat() {
    var now = Date.now();
    if (stat_cached === undefined || (now - stat_last_time) > 5) {
      var result = sendSyncMessage('FileStat', { fullPath: fullPath });
      if (result.isError)
        return result;

      stat_cached = result;
      stat_last_time = now;
      result.value.isError = result.isError;
      return result.value;
    }
    return stat_cached.value;
  }

  var getParent = function() {
    return parent;
  };
  var getReadOnly = function() {
    var status = stat();
    if (status.isError)
      return true;
    return status.readOnly;
  };
  var getIsFile = function() {
    var status = stat();
    if (status.isError)
      return false;
    return status.isFile;
  };
  var getIsDirectory = function() {
    var status = stat();
    if (status.isError)
      return false;
    return status.isDirectory;
  };
  var getCreatedDate = function() {
    var status = stat();
    if (status.isError)
      return null;
    return new Date(status.created * 1000);
  };
  var getModifiedDate = function() {
    var status = stat();
    if (status.isError)
      return null;
    return new Date(status.modified * 1000);
  };
  var getPath = function() {
    var lastSlashIndex = fullPath.lastIndexOf('/');
    if (lastSlashIndex < 0)
      return fullPath;
    return fullPath.slice(0, lastSlashIndex + 1);
  };
  var getName = function() {
    var lastSlashIndex = fullPath.lastIndexOf('/');
    if (lastSlashIndex < 0)
      return '';
    return fullPath.substr(lastSlashIndex + 1);
  };
  var getFullPath = function() {
    return fullPath;
  };
  var getFileSize = function() {
    var status = stat();
    if (status.isError)
      return 0;
    if (status.isDirectory)
      return undefined;
    return status.size;
  };
  var getLength = function() {
    var status = stat();
    if (status.isError)
      return 0;
    if (status.isDirectory)
      return status.length;
    return undefined;
  };

  Object.defineProperties(this, {
    'parent': { get: getParent, enumerable: true },
    'readOnly': { get: getReadOnly, enumerable: true },
    'isFile': { get: getIsFile, enumerable: true },
    'isDirectory': { get: getIsDirectory, enumerable: true },
    'created': { get: getCreatedDate, enumerable: true },
    'modified': { get: getModifiedDate, enumerable: true },
    'path': { get: getPath, enumerable: true },
    'name': { get: getName, enumerable: true },
    'fullPath': { get: getFullPath, enumerable: true },
    'fileSize': { get: getFileSize, enumerable: true },
    'length': { get: getLength, enumerable: true }
  });
}

File.prototype.toURI = function() {
  var status = sendSyncMessage('FileGetURI', { fullPath: this.fullPath });

  if (status.isError)
    return '';
  return status.value;
};

File.prototype.listFiles = function(onsuccess, onerror, filter) {
  if (!(onsuccess instanceof Function))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  if (onerror !== null && !(onerror instanceof Function) &&
      arguments.length > 1)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  if (filter !== null && typeof(filter) !== 'object' && arguments.length > 2)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  postMessage({
    cmd: 'FileListFiles',
    fullPath: this.fullPath,
    filter: filter ? JSON.stringify(filter) : ''
  }, function(result) {
    if (result.isError) {
      if (onerror)
        onerror(new tizen.WebAPIError(result.errorCode));
    } else if (onsuccess) {
      var file_list = [];

      for (var i = 0; i < result.value.length; i++)
        file_list.push(new File(result.value[i], getFileParent(result.value[i])));

      onsuccess(file_list);
    }
  }.bind(this));
};

File.prototype.openStream = function(mode, onsuccess, onerror, encoding) {
  if (!(onsuccess instanceof Function))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  if (onerror !== null && !(onerror instanceof Function) &&
      arguments.length > 2)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  if (mode == null)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  mode = get_valid_mode(mode);
  encoding = encoding || 'UTF-8';
  if (!is_string(encoding) || !(encoding.toUpperCase() in encodings))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  postMessage({
    cmd: 'FileOpenStream',
    fullPath: this.fullPath,
    mode: mode,
    encoding: encoding
  }, function(result) {
    if (result.isError) {
      if (onerror)
        onerror(new tizen.WebAPIError(result.errorCode));
    } else if (onsuccess) {
      onsuccess(new FileStream(result.streamID));
    }
  });
};

File.prototype.readAsText = function(onsuccess, onerror, encoding) {
  if (!(onsuccess instanceof Function))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  if (onerror !== null && !(onerror instanceof Function) &&
      arguments.length > 1)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  if ((arguments.length > 2 && encoding !== null && !is_string(encoding)))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var streamOpened = function(stream) {
    // number of characters is less than or equal to number of bytes
    onsuccess(stream.read(stream.bytesAvailable));
    stream.close();
  };
  var streamError = function(error) {
    if (onerror)
      onerror(error);
  };

  if (this.isDirectory) {
    streamError(new tizen.WebAPIException(tizen.WebAPIException.IO_ERR));
    return;
  }

  this.openStream('r', streamOpened, streamError, encoding);
};

File.prototype.copyTo = function(originFilePath, destinationFilePath,
    overwrite, onsuccess, onerror) {
  if (!this.isDirectory)
    onerror(new tizen.WebAPIException(tizen.WebAPIException.IO_ERR));
  // originFilePath, destinationFilePath - full virtual file path
  if (onsuccess !== null && !(onsuccess instanceof Function) &&
      arguments.length > 3)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  if (onerror !== null && !(onerror instanceof Function) &&
      arguments.length > 4)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  if (!is_string(originFilePath) || !is_string(destinationFilePath)) {
    onerror(new tizen.WebAPIException(tizen.WebAPIException.NOT_FOUND_ERR));
    return;
  }

  if (originFilePath.indexOf('./') >= 0 || destinationFilePath.indexOf('./') >= 0) {
    onerror(new tizen.WebAPIException(tizen.WebAPIException.NOT_FOUND_ERR));
    return;
  }

  if (originFilePath.indexOf(this.fullPath) < 0) {
    onerror(new tizen.WebAPIException(tizen.WebAPIException.NOT_FOUND_ERR));
    return;
  }

  postMessage({
    cmd: 'FileCopyTo',
    originFilePath: originFilePath,
    destinationFilePath: destinationFilePath,
    overwrite: overwrite
  }, function(result) {
    if (result.isError) {
      if (onerror) {
        onerror(new tizen.WebAPIException(result.errorCode));
      }
    } else if (onsuccess) {
      onsuccess();
    }
  });
};

File.prototype.moveTo = function(originFilePath, destinationFilePath,
    overwrite, onsuccess, onerror) {
  if (!this.isDirectory)
    onerror(new tizen.WebAPIException(tizen.WebAPIException.IO_ERR));
  // originFilePath, destinationFilePath - full virtual file path
  if (onsuccess !== null && !(onsuccess instanceof Function) &&
      arguments.length > 3)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  if (onerror !== null && !(onerror instanceof Function) &&
      arguments.length > 4)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  if (!is_string(originFilePath) || !is_string(destinationFilePath)) {
    onerror(new tizen.WebAPIException(tizen.WebAPIException.NOT_FOUND_ERR));
    return;
  }

  if (originFilePath.indexOf('./') >= 0 || destinationFilePath.indexOf('./') >= 0) {
    onerror(new tizen.WebAPIException(tizen.WebAPIException.NOT_FOUND_ERR));
    return;
  }

  if (originFilePath.indexOf(this.fullPath) < 0) {
    onerror(new tizen.WebAPIException(tizen.WebAPIException.NOT_FOUND_ERR));
    return;
  }

  postMessage({
    cmd: 'FileMoveTo',
    originFilePath: originFilePath,
    destinationFilePath: destinationFilePath,
    overwrite: overwrite
  }, function(result) {
    if (result.isError) {
      if (onerror) {
        onerror(new tizen.WebAPIException(result.errorCode));
      }
    } else if (onsuccess) {
      onsuccess();
    }
  });
};

File.prototype.createDirectory = function(relativeDirPath) {
  if (!this.isDirectory)
    onerror(new tizen.WebAPIException(tizen.WebAPIException.IO_ERR));
  if (relativeDirPath.indexOf('./') >= 0)
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);

  var status = sendSyncMessage('FileCreateDirectory', {
    fullPath: this.fullPath,
    relativeDirPath: relativeDirPath
  });

  if (status.isError)
    throw new tizen.WebAPIException(status.errorCode);
  else
    return new File(status.value, getFileParent(status.value));
};

File.prototype.createFile = function(relativeFilePath) {
  if (!this.isDirectory)
    onerror(new tizen.WebAPIException(tizen.WebAPIException.IO_ERR));
  if (relativeFilePath.indexOf('./') >= 0)
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);

  var status = sendSyncMessage('FileCreateFile', {
    fullPath: this.fullPath,
    relativeFilePath: relativeFilePath
  });

  if (status.isError)
    throw new tizen.WebAPIException(status.errorCode);
  else
    return new File(status.value, getFileParent(status.value));
};

File.prototype.resolve = function(relativeFilePath) {
  if (!this.isDirectory)
    onerror(new tizen.WebAPIException(tizen.WebAPIException.IO_ERR));
  var status = sendSyncMessage('FileResolve', {
    fullPath: this.fullPath,
    relativeFilePath: relativeFilePath
  });

  if (status.isError)
    throw new tizen.WebAPIException(status.errorCode);

  return new File(status.value, getFileParent(status.value));
};

File.prototype.deleteDirectory = function(directoryPath, recursive, onsuccess, onerror) {
  if (!this.isDirectory)
    onerror(new tizen.WebAPIException(tizen.WebAPIException.IO_ERR));
  // directoryPath - full virtual directory path
  if (onsuccess !== null && !(onsuccess instanceof Function) &&
      arguments.length > 2)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  if (onerror !== null && !(onerror instanceof Function) &&
      arguments.length > 3)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  if (directoryPath.indexOf(this.fullPath) < 0 && onerror) {
    onerror(new tizen.WebAPIError(tizen.WebAPIException.NOT_FOUND_ERR));
    return;
  }

  postMessage({
    cmd: 'FileDeleteDirectory',
    directoryPath: directoryPath,
    recursive: !!recursive
  }, function(result) {
    if (result.isError) {
      if (onerror)
        onerror(new tizen.WebAPIError(result.errorCode));
    } else if (onsuccess) {
      onsuccess();
    }
  });
};

File.prototype.deleteFile = function(filePath, onsuccess, onerror) {
  if (!this.isDirectory)
    onerror(new tizen.WebAPIException(tizen.WebAPIException.IO_ERR));
  // filePath - full virtual file path
  if (onsuccess !== null && !(onsuccess instanceof Function) &&
      arguments.length > 1)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  if (onerror !== null && !(onerror instanceof Function) &&
      arguments.length > 2)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  if (filePath.indexOf(this.fullPath) < 0 && onerror) {
    onerror(new tizen.WebAPIError(tizen.WebAPIException.NOT_FOUND_ERR));
    return;
  }

  postMessage({
    cmd: 'FileDeleteFile',
    filePath: filePath
  }, function(result) {
    if (result.isError) {
      if (onerror)
        onerror(new tizen.WebAPIError(result.errorCode));
    } else if (onsuccess) {
      onsuccess();
    }
  });
};

(function() {
  exports = new FileSystemManager();
})();

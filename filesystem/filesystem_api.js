// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var _callbacks = {};
var _next_reply_id = 0;

var getNextReplyId = function() {
  return _next_reply_id++;
};

var postMessage = function(msg, callback) {
  var reply_id = getNextReplyId();
  _callbacks[reply_id] = callback;
  msg.reply_id = reply_id;
  extension.postMessage(JSON.stringify(msg));
};

extension.setMessageListener(function(json) {
  var msg = JSON.parse(json);
  var reply_id = msg.reply_id;
  var callback = _callbacks[reply_id];
  if (typeof(callback) === 'function') {
    callback(msg);
    delete msg.reply_id;
    delete _callbacks[reply_id];
  } else {
    console.log('Invalid reply_id from Tizen Filesystem: ' + reply_id);
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
  postMessage({
    cmd: 'FileSystemManagerResolve',
    location: location,
    mode: mode
  }, function(result) {
    if (result.isError && typeof(onerror) === 'function')
      onerror(JSON.stringify(result));
    else if (typeof(onsuccess) === 'function')
      onsuccess(new File(result.realPath));
  });
};

FileSystemManager.prototype.getStorage = function(label, onsuccess, onerror) {
  postMessage({
    cmd: 'FileSystemManagerGetStorage',
    location: location,
    mode: mode
  }, function(result) {
    if (result.error != 0) {
      if (onerror)
        onerror(result);
      else if (onsuccess)
        onsuccess(new FileSystemStorage(result.label,
            result.type, result.state));
    }
  });
};

FileSystemManager.prototype.listStorages = function(onsuccess, onerror) {
  postMessage({
    cmd: 'FileSystemManagerListStorages',
    location: location,
    mode: mode
  }, function(result) {
    if (result.error != 0) {
      if (onerror) {
        onerror(result);
      } else if (onsuccess) {
        var storages = [];

        for (var i = 0; i < result.storages.length; i++) {
          var storage = results.storages[i];
          storages.push(new FileSystemStorage(storage.label,
              storage.type, storage.state));
        }

        onsuccess(storages);
      }
    }
  });
};

FileSystemManager.prototype.addStorageStateChangeListener = function(onsuccess, onerror) {
  /* FIXME(leandro): Implement this. */
  onsuccess(0);
};

FileSystemManager.prototype.removeStorageStateChangeListener = function(watchId) {
  /* FIXME(leandro): Implement this. */
};

function FileFilter(name, startModified, endModified, startCreated, endCreated) {
  var self = {
    toString: function() {
      return JSON.stringify(this);
    }
  };
  Object.defineProperties(self, {
    'name': { writable: false, value: name, enumerable: true },
    'startModified': { writable: false, value: startModified, enumerable: true },
    'endModified': { writable: false, value: endModified, enumerable: true },
    'startCreated': { writable: false, value: startCreated, enumerable: true },
    'endCreated': { writable: false, value: endCreated, enumerable: true }
  });
  return self;
}

function FileStream(fileDescriptor) {
  this.fileDescriptor = fileDescriptor;
}

FileStream.prototype.close = function() {
  sendSyncMessage('FileStreamClose', {
    fileDescriptor: this.fileDescriptor
  });
  this.fileDescriptor = -1;
};

FileStream.prototype.read = function(charCount) {
  var result = sendSyncMessage('FileStreamRead', {
    fileDescriptor: this.fileDescriptor,
    charCount: charCount
  });
  if (result.isError)
    return '';
  return result.value;
};

FileStream.prototype.readBytes = function(byteCount) {
  return sendSyncMessage('FileStreamReadBytes', {
    fileDescriptor: this.fileDescriptor,
    byteCount: byteCount
  });
};

FileStream.prototype.readBase64 = function(byteCount) {
  return sendSyncMessage('FileStreamReadBase64', {
    fileDescriptor: this.fileDescriptor,
    byteCount: byteCount
  });
};

FileStream.prototype.write = function(stringData) {
  return sendSyncMessage('FileStreamWrite', {
    fileDescriptor: this.fileDescriptor,
    stringData: stringData
  });
};

FileStream.prototype.writeBytes = function(byteData) {
  return sendSyncMessage('FileStreamWriteBytes', {
    fileDescriptor: this.fileDescriptor,
    byteData: byteData
  });
};

FileStream.prototype.writeBase64 = function(base64Data) {
  return sendSyncMessage('FileStreamWriteBase64', {
    fileDescriptor: this.fileDescriptor,
    base64Data: base64Data
  });
};

function File(path, parent) {
  this.path = path;
  this.parent = parent;

  var stat_cached = undefined;
  var stat_last_time = undefined;

  function getPathAndParent() {
    var _path = path.lastIndexOf('/');
    if (_path < 0) {
      return {
        path: _path,
        parent: parent ? parent.path : ''
      };
    }

    return {
      path: path.substr(_path + 1),
      parent: parent ? parent.path : path.substr(0, _path)
    };
  }

  function stat() {
    var now = Date.now();
    if (stat_cached === undefined || (now - stat_last_time) > 5) {
      var args = getPathAndParent();
      var result = sendSyncMessage('FileStat', args);
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
    return path;
  };
  var getName = function() {
    var fullPath = getFullPath();
    var lastSlashIndex = fullPath.lastIndexOf('/');
    if (lastSlashIndex < 0)
      return fullPath;
    return fullPath.substr(lastSlashIndex + 1);
  };
  var getFullPath = function() {
    if (path[0] == '/')
      return path;
    var status = sendSyncMessage('FileGetFullPath', { path: path });
    if (status.isError)
      return path;
    return status.value;
  };
  var getFileSize = function() {
    var status = stat();
    if (status.isError)
      return 0;
    if (status.isDirectory)
      return 0;
    return status.size;
  };
  var getLength = function() {
    if (getIsFile())
      return 1;
    return files.length;
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
  var realPathStatus = sendSyncMessage('FileGetFullPath', {
    path: this.path
  });
  if (!realPathStatus.isError)
    return 'file://' + realPathStatus.fullPath;
  return null;
};

File.prototype.listFiles = function(onsuccess, onerror, filter) {
  if (!(onsuccess instanceof Function))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  if (typeof(filter) !== 'undefined' && !(filter instanceof FileFilter))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  postMessage({
    cmd: 'FileListFiles',
    path: this.path,
    filter: filter ? filter.toString() : ''
  }, function(result) {
    if (result.isError) {
      if (!onerror || !(onerror instanceof Function))
        return;
      onerror(result);
    } else {
      var file_list = [];

      for (var i = 0; i < result.value.length; i++)
        file_list.push(new File(result.value[i], this));

      onsuccess(file_list);
    }
  }.bind(this));
};

File.prototype.openStream = function(mode, onsuccess, onerror, encoding) {
  postMessage({
    cmd: 'FileOpenStream',
    filePath: this.path,
    mode: mode,
    encoding: encoding
  }, function(result) {
    if (result.isError) {
      if (onerror)
        onerror(result);
    } else if (onsuccess) {
      onsuccess(new FileStream(result.fileDescriptor));
    }
  });
};

File.prototype.readAsText = function(onsuccess, onerror, encoding) {
  var streamOpened = function(stream) {
    onsuccess(stream.read());
    stream.close();
  };
  var streamError = function(error) {
    if (onerror)
      onerror(error);
  };

  this.openStream('r', streamOpened, streamError, encoding);
};

File.prototype.copyTo = function(originFilePath, destinationFilePath,
    overwrite, onsuccess, onerror) {
  var status = sendSyncMessage('FileCopyTo', {
    originFilePath: originFilePath,
    destinationFilePath: destinationFilePath,
    overwrite: overwrite
  });

  if (status.isError) {
    if (onerror)
      onerror(status);
  } else {
    onsuccess(status);
  }
};

File.prototype.moveTo = function(originFilePath, destinationFilePath,
    overwrite, onsuccess, onerror) {
  var status = sendSyncMessage('FileMoveTo', {
    originFilePath: originFilePath,
    destinationFilePath: destinationFilePath,
    overwrite: overwrite
  });

  if (status.isError) {
    if (onerror)
      onerror(status);
  } else {
    onsuccess(status);
  }
};

File.prototype.createDirectory = function(relative) {
  var status = sendSyncMessage('FileCreateDirectory', {
    path: this.path,
    relative: relative
  });

  if (status.isError) {
    throw new tizen.WebAPIException(status.errorCode);
  } else {
    return new File(status.path);
  }
};

File.prototype.createFile = function(relative) {
  var status = sendSyncMessage('FileCreateFile', {
    path: this.path,
    relative: relative
  });
  if (status.isError) {
    throw new tizen.WebAPIException(status.errorCode);
  } else {
    return new File(status.value, this);
  }
};

File.prototype.resolve = function(relative) {
  var status = sendSyncMessage('FileResolve', {
    path: this.path,
    relative: relative
  });

  if (status.isError)
    throw new tizen.WebAPIException(status.errorCode);

  return new File(status.value);
};

File.prototype.deleteDirectory = function(directoryPath, recursive, onsuccess, onerror) {
  postMessage({
    cmd: 'FileDeleteDirectory',
    directoryPath: directoryPath,
    path: this.path,
    recursive: !!recursive
  }, function(result) {
    if (result.isError) {
      if (onerror) {
        var error = new tizen.WebAPIError(tizen.WebAPIException.UNKNOWN_ERR);
        onerror(error);
      }
    } else if (onsuccess) {
      onsuccess();
    }
  });
};

File.prototype.deleteFile = function(filePath, onsuccess, onerror) {
  postMessage({
    cmd: 'FileDeleteFile',
    path: this.path,
    filePath: filePath
  }, function(result) {
    if (result.isError) {
      if (onerror)
        onerror(result);
    } else if (onsuccess) {
      onsuccess();
    }
  });
};


(function() {
  var manager = new FileSystemManager();
  exports.resolve = manager.resolve;
  exports.getStorage = manager.getStorage;
  exports.listStorages = manager.listStorages;
  exports.addStorageStateChangeListener = manager.addStorageStateChangeListener;
  exports.removeStorageStateChangeListener = manager.removeStorageStateChangeListener;
  exports.maxPathLength = manager.maxPathLength;
})();

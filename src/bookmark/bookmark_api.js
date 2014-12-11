// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is organized in two parts. In the first part, there's the API
// implemented, containing all the error checking code according to what's
// expected for the Tizen API. The actual functionality is implemented by
// relying on a 'BookmarkProvider' object.
//
// The second part is the implementation of the BookmarkProvider, that will
// call the native code.

var v8tools = requireNative('v8tools');

function defineReadOnlyProperty(object, key, value) {
  Object.defineProperty(object, key, {
    configurable: false,
    writable: false,
    value: value
  });
}

function _sendSyncMessage(cmd, id, title, url, parent_id, type) {
  var msg = {
    'cmd': cmd,
    'id': Number(id),
    'title': title,
    'url': url,
    'parent_id': Number(parent_id),
    'type': Number(type)
  };
  return JSON.parse(extension.internal.sendSyncMessage(JSON.stringify(msg)));
}

defineReadOnlyProperty(tizen, 'BookmarkItem', function(title, url) {
  // FIXME(cmarcelo): This is a best effort to ensure that this function is
  // called as a constructor only. We may need to implement some native
  // primitive to ensure that.
  if (!this || this.constructor != tizen.BookmarkItem)
    throw new TypeError;
  defineReadOnlyProperty(this, 'title', title);
  defineReadOnlyProperty(this, 'url', url);
  defineReadOnlyProperty(this, 'parent', undefined);
  defineReadOnlyProperty(this, '_id', null);
});

defineReadOnlyProperty(tizen, 'BookmarkFolder', function(title) {
  // FIXME(cmarcelo): This is a best effort to ensure that this function is
  // called as a constructor only. We may need to implement some native
  // primitive to ensure that.
  if (!this || this.constructor != tizen.BookmarkFolder)
    throw new TypeError;
  defineReadOnlyProperty(this, 'title', title);
  defineReadOnlyProperty(this, 'parent', undefined);
  defineReadOnlyProperty(this, '_id', null);
});

function isBookmarkType(object) {
  return (object instanceof tizen.BookmarkItem) || (object instanceof tizen.BookmarkFolder);
}

// The provider object is used by the public API functions to get the
// actual data. See implementation below.
var provider = new BookmarkProvider();

exports.get = function(parentFolder, recursive) {
  if (arguments.length == 0 || parentFolder === null) {
    return provider.getFolderItems(provider.getRootID(), recursive);
  }

  if (!(parentFolder instanceof tizen.BookmarkFolder))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  if (parentFolder._id == null)
    throw new tizen.WebAPIException(tizen.WebAPIException.NOT_FOUND_ERR);

  var result = provider.getFolderItems(parentFolder._id, recursive);

  if (result == null)
    throw new tizen.WebAPIException(tizen.WebAPIException.NOT_FOUND_ERR);
  return result;
};

exports.add = function(bookmark, parentFolder) {
  if (!isBookmarkType(bookmark))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  // Note: we use a strict "=== null" check here since we want only null case
  // but not undefined. We do this because test expects different behavior from
  // not passing an argument and passing undefined to it.
  if (arguments.length == 1 || parentFolder === null) {
    if (bookmark._id != null)
      throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);

    if (!provider.addToFolder(bookmark, provider.getRootID()))
      throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);
    return;
  }

  if (!(parentFolder instanceof tizen.BookmarkFolder))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  if (parentFolder._id == null)
    throw new tizen.WebAPIException(tizen.WebAPIException.NOT_FOUND_ERR);

  if (!provider.addToFolder(bookmark, parentFolder._id))
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);
};

exports.remove = function(bookmark) {
  if (arguments.length == 0 || bookmark === null) {
    provider.removeAll();
    return;
  }

  if (!isBookmarkType(bookmark))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  if (bookmark._id == null)
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);

  provider.removeBookmark(bookmark);
};

// Implementation of BookmarkProvider. It contains the operations needed by the
// public API functions. We refer to the folders here by their IDs in many
// functions.

function BookmarkProvider() {}

BookmarkProvider.prototype.addToFolder = function(bookmark, parentID) {
  var isBookmarkFolder = bookmark instanceof tizen.BookmarkFolder;
  var item = _sendSyncMessage('AddBookmark', null, bookmark.title, bookmark.url,
                              parentID, isBookmarkFolder ? 1 : 0);

  if (item.error)
    return false;

  v8tools.forceSetProperty(bookmark, '_id', item.id);
  v8tools.forceSetProperty(bookmark, 'parent', this.getFolder(parentID));

  return true;
};

BookmarkProvider.prototype.getFolder = function(id) {
  if (arguments.length == 0 || id <= 0)
    return null;

  // root folder might have any ID number (not only 0)
  if (id == this.getRootID())
    return null;

  var folder = _sendSyncMessage('GetFolder', id);

  var obj = new tizen.BookmarkFolder(folder.value[0].title);
  v8tools.forceSetProperty(obj, '_id', folder.value[0].id);
  v8tools.forceSetProperty(obj, 'parent', this.getFolder(folder.value[0].folder_id));

  return obj;
};

BookmarkProvider.prototype.getFolderItems = function(id, recursive) {
  var folder = _sendSyncMessage('GetFolderItems', id);

  var item;
  var result = [];
  var len = folder.value.length;
  for (var i = 0; item = folder.value[i], i < len; i++) {
    if (item.is_folder == 0) {
      var obj = new tizen.BookmarkItem(item.title, item.address);
      v8tools.forceSetProperty(obj, '_id', item.id);
      v8tools.forceSetProperty(obj, 'parent', this.getFolder(item.folder_id));
      result.push(obj);
    } else {
      var obj = new tizen.BookmarkFolder(item.title);
      v8tools.forceSetProperty(obj, '_id', item.id);
      v8tools.forceSetProperty(obj, 'parent', this.getFolder(item.folder_id));
      result.push(obj);
      if (recursive)
        result = result.concat(this.getFolderItems(item.id, true));
    }
  }

  return result;
};

BookmarkProvider.prototype.removeAll = function() {
  if (_sendSyncMessage('RemoveAll').error)
    throw new tizen.WebAPIException(tizen.WebAPIException.SECURITY_ERR);
};

BookmarkProvider.prototype.removeBookmark = function(bookmark) {
  if (_sendSyncMessage('RemoveBookmark', bookmark._id).error) {
    throw new tizen.WebAPIException(tizen.WebAPIException.SECURITY_ERR);
    return null;
  }

  v8tools.forceSetProperty(bookmark, '_id', null);
  v8tools.forceSetProperty(bookmark, 'parent', undefined);
};

BookmarkProvider.prototype.getRootID = function() {
  var rootID = _sendSyncMessage('GetRootID');

  if (rootID.error) {
    throw new tizen.WebAPIException(tizen.WebAPIException.SECURITY_ERR);
    return null;
  }

  return Number(rootID.value);
};

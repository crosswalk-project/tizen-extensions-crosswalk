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
var provider = new FakeBookmarkProvider();

exports.get = function(parentFolder, recursive) {
  if (arguments.length == 0 || parentFolder === null) {
    return recursive ? provider.getFolderRecursive(0) : provider.getFolder(0);
  }

  if (!(parentFolder instanceof tizen.BookmarkFolder))
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  if (parentFolder._id == null)
    throw new tizen.WebAPIException(tizen.WebAPIException.NOT_FOUND_ERR);

  var result;
  if (recursive)
    result = provider.getFolderRecursive(parentFolder._id);
  else
    result = provider.getFolder(parentFolder._id);

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

    if (!provider.addToFolder(bookmark, 0))
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
//
// TODO(cmarcelo): This is a FAKE BOOKMARK PROVIDER, that do not query/store the
// bookmark data in the real database. It should be replaced with an appropriate
// provider that communicates to the native side of the extension.

function FakeBookmarkProvider() {
  this.next_id = 1;
  this.folders = {};
  this.folders[0] = {
    bookmark: null,
    contents: []
  };
  this.urls = {};
}

function containsWithTitle(array, title) {
  var i;
  for (i = 0; i < array.length; i++) {
    if (array[i].title === title)
      return true;
  }
  return false;
}

FakeBookmarkProvider.prototype.addToFolder = function(bookmark, parentID) {
  var parent = this.folders[parentID];
  var isBookmarkItem = bookmark instanceof tizen.BookmarkItem;
  if (!parent)
    return false;

  if (isBookmarkItem && this.urls[bookmark.url])
    return false;

  if (containsWithTitle(parent.contents, bookmark.title))
    return false;

  v8tools.forceSetProperty(bookmark, 'parent', parent.bookmark);
  v8tools.forceSetProperty(bookmark, '_id', this.next_id);
  this.next_id += 1;
  parent.contents.push(bookmark);
  if (isBookmarkItem)
    this.urls[bookmark.url] = true;

  if (bookmark instanceof tizen.BookmarkFolder) {
    this.folders[bookmark._id] = {
      bookmark: bookmark,
      contents: []
    };
  }

  return true;
};

FakeBookmarkProvider.prototype.getFolder = function(id) {
  var parent = this.folders[id];
  if (!parent)
    return null;

  var result = [];
  var folder = parent.contents;
  var i;

  for (i = 0; i < folder.length; i++)
    result.push(folder[i]);
  return result;
};

FakeBookmarkProvider.prototype.getFolderRecursive = function(id) {
  var parent = this.folders[id];
  if (!parent)
    return null;

  var result = [];
  var folder = parent.contents;
  var i;

  for (i = 0; i < folder.length; i++) {
    result.push(folder[i]);
    if (folder[i] instanceof tizen.BookmarkFolder)
      Array.prototype.push.apply(result, this.getFolderRecursive(folder[i]._id));
  }

  return result;
};

FakeBookmarkProvider.prototype.removeAll = function() {
  var root = this.folders[0].contents.slice();
  var i;

  for (i = 0; i < root.length; i++) {
    this.removeBookmark(root[i]);
  }
};

FakeBookmarkProvider.prototype.removeBookmark = function(bookmark) {
  var i;
  var id = bookmark._id;

  if (bookmark instanceof tizen.BookmarkFolder) {
    var children = this.folders[id].contents.splice();
    for (i = 0; i < children; i++)
      this.removeBookmark(children[i]);
  } else {
    delete this.urls[bookmark.url];
  }

  var folder;
  if (bookmark.parent == null)
    folder = this.folders[0];
  else
    folder = this.folders[bookmark.parent._id];

  v8tools.forceSetProperty(bookmark, 'parent', undefined);
  v8tools.forceSetProperty(bookmark, '_id', null);

  for (i = 0; i < folder.contents.length; i++) {
    if (bookmark === folder.contents[i]) {
      folder.contents.splice(i, 1);
      break;
    }
  }
};

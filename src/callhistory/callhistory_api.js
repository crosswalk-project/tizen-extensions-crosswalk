// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// CallHistory WebIDL specification
// https://developer.tizen.org/dev-guide/2.2.1/org.tizen.web.device.apireference/tizen/callhistory.html

function error(txt) {
  var text = txt instanceof Object ? toPrintableString(txt) : txt;
  console.log('\n[CallHist JS] Error: ' + txt);
}

// print an Object into a string; used for logging
function toPrintableString(o) {
  if (!(o instanceof Object) && !(o instanceof Array))
    return o;
  var out = '{ ';
  for (var i in o) {
    out += i + ': ' + toPrintableString(o[i]) + ', ';
  }
  out += '}';
  return out;
}

function isValidString(value) {
  return typeof(value) === 'string' || value instanceof String;
}

function isValidInt(value) {
  return isFinite(value) && !isNaN(parseInt(value));
}

function isValidFunction(value) {
  return (value && (value instanceof Function));
}

function throwTizenTypeMismatch() {
  throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
}

function throwTizenInvalidValue() {
  throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);
}

function throwTizenNotFound() {
  throw new tizen.WebAPIException(tizen.WebAPIException.NOT_FOUND_ERR);
}

function throwTizenUnknown() {
  throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);
}

function throwTizenException(e) {
  if (e instanceof TypeError)
    throwTizenTypeMismatch();
  else if (e instanceof RangeError)
    throwTizenInvalidValue();
  else
    throwTizenUnknownError();
}

var callh_listeners = {};
var callh_listener_id = 0;
var callh_listeners_count = 0;

var callh_onsuccess = {};
var callh_onerror = {};
var callh_next_reply_id = 0;

var getNextReplyId = function() {
  return callh_next_reply_id++;
};

// send a JSON message to the native extension code
function postMessage(msg, onsuccess, onerror) {
  var reply_id = getNextReplyId();
  msg.reply_id = reply_id;
  callh_onsuccess[reply_id] = onsuccess;
  if (isValidFunction(onerror))
    callh_onerror[reply_id] = onerror;
  var sm = JSON.stringify(msg);
  extension.postMessage(sm);
}

function handleReply(msg) {
  // reply_id can be 0
  if (!msg || msg.reply_id == null || msg.reply_id == undefined) {
    error('Listener error for reply, called with: \n' + msg);
    throwTizenUnknown();
  }
  if (msg.errorCode != tizen.WebAPIError.NO_ERROR) {
    var onerror = callh_onerror[msg.reply_id];
    if (isValidFunction(onerror)) {
      onerror(new tizen.WebAPIError(msg.errorCode));
      delete callh_onerror[msg.reply_id];
    } else {
      error('Error: error callback is not a function');
    }
    return;
  }
  var onsuccess = callh_onsuccess[msg.reply_id];
  if (isValidFunction(onsuccess)) {
    onsuccess(msg.result);
    delete callh_onsuccess[msg.reply_id];
  } else {
    error('Error: success callback is not a function');
  }
}

function handleNotification(msg) {
  if (msg.errorCode != tizen.WebAPIError.NO_ERROR) {
    error('Error code in listener callback');
    return;
  }
  for (var key in callh_listeners) {
    var cb = callh_listeners[key];
    if (!cb) {
      error('No listener object found for handle ' + key);
      return;
    }

    if (cb.onadded && msg.added && msg.added.length > 0)
      cb.onadded(msg.added);

    if (cb.onchanged && msg.changed && msg.changed.length > 0)
      cb.onchanged(msg.changed);

    if (cb.onremoved && msg.deleted && msg.deleted.length > 0)
      cb.onremoved(msg.deleted);
  }
}

// handle the JSON messages sent from the native extension code to JS
// including replies and change notifications
extension.setMessageListener(function(json) {
  var msg = JSON.parse(json);
  if (!msg || !msg.errorCode || !msg.cmd) {
    error('Listener error, called with: \n' + json);
    return;
  }

  if (msg.cmd == 'reply') {
    handleReply(msg);
  } else if (msg.cmd == 'notif') {
    handleNotification(msg);
  } else {
    error('invalid JSON message from extension: ' + json);
  }
});

function isValidFilter(f) {
  return (f instanceof tizen.AttributeFilter) ||
         (f instanceof tizen.AttributeRangeFilter) ||
         (f instanceof tizen.CompositeFilter);
}

exports.find = function(successCallback, errorCallback, filter, sortMode, limit, offset) {
  if (!isValidFunction(successCallback))
    throwTizenTypeMismatch();

  if (arguments.length > 1 && errorCallback && !(errorCallback instanceof Function))
    throwTizenTypeMismatch();

  if (arguments.length > 2 && filter && !isValidFilter(filter))
    throwTizenTypeMismatch();

  if (arguments.length > 3 && sortMode && !(sortMode instanceof tizen.SortMode))
    throwTizenTypeMismatch();

  if (arguments.length > 4 && limit && !isValidInt(limit))
    throwTizenTypeMismatch();

  if (arguments.length > 5 && offset && !isValidInt(offset))
    throwTizenTypeMismatch();

  var cmd = {
    cmd: 'find',
    filter: filter,
    sortMode: sortMode,
    limit: limit,
    offset: offset
  };
  postMessage(cmd, successCallback, errorCallback);
};

exports.remove = function(callEntry) {
  var uid = callEntry.uid;
  if (callEntry.uid == undefined)
    throwTizenUnknown();

  postMessage({ cmd: 'remove', uid: uid },
              function() { print('Entry removed: uid = ' + uid); },
              function(err) { error(err.name + ': ' + err.message);});
};

exports.removeBatch = function(callEntryList, successCallback, errorCallback) {
  if (!callEntryList || callEntryList.length == 0)
    throwTizenInvalidValue();

  if (!successCallback && !(successCallback instanceof Function))
    throwTizenTypeMismatch();

  if (!errorCallback && (!successCallback || !(errorCallback instanceof Function)))
    throwTizenTypeMismatch();

  var uids = [];  // collect uid's from all entries
  for (var i = 0; i < callEntryList.length; i++) {
    if (callEntryList[i].uid == undefined)
      throwTizenTypeMismatch();
    uids.push(callEntryList[i].uid);
  }

  postMessage({ cmd: 'removeBatch', uids: uids }, successCallback, errorCallback);
};

exports.removeAll = function(successCallback, errorCallback) {
  if (!successCallback && !(successCallback instanceof Function))
    throwTizenTypeMismatch();

  if (!errorCallback && (!successCallback || !(errorCallback instanceof Function)))
    throwTizenTypeMismatch();

  postMessage({ cmd: 'removeAll' }, successCallback, errorCallback);
};

exports.addChangeListener = function(obs) {
  if (!obs || !(isValidFunction(obs.onadded) ||
      isValidFunction(obs.onchanged) || isValidFunction(obs.onremoved)))
    throwTizenTypeMismatch();

  for (var key in callh_listeners) {  // if the same object was registered
    if (callh_listeners[key] == obs &&
        callh_listeners[key].onadded == obs.onadded &&
        callh_listeners[key].onremoved == obs.onremoved &&
        callh_listeners[key].onchanged == obs.onchanged) {
      return key;
    }
  }

  callh_listeners[++callh_listener_id] = obs;
  callh_listeners_count++;
  if (callh_listeners_count == 1) {
    postMessage({ cmd: 'addListener' },
                function() { print('Listener registered'); },
                function(err) { error(err.name + ': ' + err.message);});
  }
  return callh_listener_id;
};

exports.removeChangeListener = function(handle) {
  if (!isValidInt(handle)) {
    throwTizenTypeMismatch();
  }

  var obs = callh_listeners[handle];
  if (!obs) {
    throwTizenInvalidValue();
  }

  delete callh_listeners[handle];
  callh_listeners_count--;
  if (callh_listeners_count == 0) {
    postMessage({ cmd: 'removeListener'},
                function() { print('Listener unregistered'); },
                function(err) { error(err.name + ': ' + err.message);});
  }
};

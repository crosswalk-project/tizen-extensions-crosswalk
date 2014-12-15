// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

///////////////////////////////////////////////////////////////////////////////
// Utilities
///////////////////////////////////////////////////////////////////////////////

var g_next_async_call_id = 1;
var g_async_calls = {};
var v8tools = requireNative('v8tools');
var connected_peer_ = null;

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
    enumerable: true,
    writable: false,
    value: propertyValue
  });
}

function _addHiddenProperty(obj, propertyKey, propertyValue) {
  Object.defineProperty(obj, propertyKey, {
    enumerable: false,
    writable: true,
    value: propertyValue
  });
}

function _addConstructorProperty(obj, constructor) {
  Object.defineProperty(obj, 'constructor', {
    enumerable: false,
    value: constructor
  });
}

function derive(child, parent) {
  child.prototype = Object.create(parent.prototype);
  child.prototype.constructor = child;
  _addConstructorProperty(child.prototype, child);
}

function EventTargetInterface(eventListeners, isValidType) {
  _addHiddenProperty(this, 'listeners_', eventListeners);
  _addHiddenProperty(this, 'isValidType_', isValidType);
}

EventTargetInterface.prototype.addEventListener = function(type, callback) {
  if (callback != null && typeof callback === 'function' &&
      this.isValidType_(type))
    if (~~this.listeners_[type].indexOf(callback))
      this.listeners_[type].push(callback);
};

EventTargetInterface.prototype.removeEventListener = function(type, callback) {
  if (callback != null &&
      typeof callback === 'function' && this.isValidType_(type)) {
    var index = this.listeners_[type].indexOf(callback);
    if (~index)
      this.listeners_[type].slice(index, 1);
  }
};

EventTargetInterface.prototype.dispatchEvent = function(event) {
  var handled = true;

  if (typeof event !== 'object' || !this.isValidType_(event.type))
    return false;

  this.listeners_[event.type].forEach(function(callback) {
    var res = callback(event);
    if (!res && handled)
      handled = false;
  });

  return handled;
};

///////////////////////////////////////////////////////////////////////////////
// Extension message listener
///////////////////////////////////////////////////////////////////////////////

extension.setMessageListener(function(json) {
  var msg = JSON.parse(json);
  switch (msg.cmd) {
    case 'poweron':
      v8tools.forceSetProperty(g_nfc_manager, 'powered', true);
      break;
    case 'poweroff':
      v8tools.forceSetProperty(g_nfc_manager, 'powered', false);
      break;
    case 'pollstart':
      v8tools.forceSetProperty(g_nfc_manager, 'polling', true);
      break;
    case 'pollstop':
      v8tools.forceSetProperty(g_nfc_manager, 'polling', false);
      break;
    case 'messageread':
      handleMessageRead(msg);
      break;
    case 'readNdef':
      handleReadNDEF(msg);
      break;
    case 'sendNdef':
      handleAsyncCallSuccess(msg.asyncCallId);
      break;
    case 'getPayload':
      handleAsyncCallSuccess(msg.asyncCallId, msg.result);
      break;
    case 'asyncCallError':
      handleAsyncCallError(msg);
      break;
    default:
      console.log('Received unknown command');
      break;
  }

  if (msg.cmd in NFCManagerEventNames)
    handleEvent(msg);
});

///////////////////////////////////////////////////////////////////////////////
// NFCManager
///////////////////////////////////////////////////////////////////////////////

var NFCManagerEventNames = {
  poweron: 0,
  poweroff: 1,
  pollstart: 2,
  pollstop: 3,
  tagfound: 4,
  taglost: 5,
  peerfound: 6,
  peerlost: 7
};

function handleEvent(msg) {
  if (msg.asyncCallId in g_async_calls) {
    g_async_calls[msg.asyncCallId].resolve();
    delete g_async_calls[msg.asyncCallId];
  }
  dispatchEventForName(msg.cmd);
}

function handleAsyncCallSuccess(callId, result) {
  if (callId in g_async_calls) {
    g_async_calls[callId].resolve(result);
    delete g_async_calls[callId];
  }
}

function toNDEFRecord(rec) {
  var ndef = null;
  switch (rec.recordType) {
    case 'text':
      ndef = new NDEFRecordText(rec.text, rec.languageCode, rec.encoding);
      break;
    case 'uri':
      ndef = new NDEFRecordURI(rec.uri);
      break;
    case 'media':
      ndef = new NDEFRecordMedia(rec.mimeType, rec.payload);
      break;
    case 'smartPoster':
    default:
      ndef = new NDEFRecord(rec.tnf, rec.type, rec.payload,
          rec.id, rec.recordType);
      break;
  }
  v8tools.forceSetProperty(ndef, 'payload', rec.payload);
  return ndef;
}

function handleReadNDEF(msg) {
  handleAsyncCallSuccess(msg.asyncCallId, toNDEFRecord(msg.result[0]));
}

function handleAsyncCallError(msg) {
  if (msg.asyncCallId in g_async_calls) {
    g_async_calls[msg.asyncCallId].reject(Error('Async operation failed'));
    delete g_async_calls[msg.asyncCallId];
  }
}

function dispatchEventForName(eventName) {
  var event = new CustomEvent(eventName);
  if (eventName === 'tagfound')
    _addConstProperty(event, 'tag', new NFCTag());
  else if (eventName === 'peerfound')
    _addConstProperty(event, 'peer', connected_peer_ = new NFCPeer());
  else if (eventName === 'peerlost')
    connected_peer_ = null;
  dispatchEvent(event, eventName);
}

function dispatchEvent(event, eventName) {
  g_nfc_manager.dispatchEvent(event);
  if (g_nfc_manager[eventName] &&
      typeof g_nfc_manager[eventName] === 'function')
    g_nfc_manager[eventName](event);
}

function NFCManager() {
  var nfc_manager_listeners = {};
  for (var key in NFCManagerEventNames)
    nfc_manager_listeners[key] = [];

  EventTargetInterface.call(this,
      nfc_manager_listeners,
      function(type) {return type in NFCManagerEventNames;});
  var is_powered = JSON.parse(extension.internal.sendSyncMessage(
      JSON.stringify({cmd: 'is_powered'})));
  _addConstProperty(this, 'powered', is_powered);
  _addConstProperty(this, 'polling', false);
  this.onpoweron = null;
  this.onpoweroff = null;
  this.onpollstart = null;
  this.onpollstop = null;
  this.ontagfound = null;
  this.ontaglost = null;
  this.onpeerfound = null;
  this.onpeerlost = null;
}
derive(NFCManager, EventTargetInterface);

NFCManager.prototype.powerOn = function() {
  return createPromise({'cmd': 'poweron'});
};

NFCManager.prototype.powerOff = function() {
  return createPromise({'cmd': 'poweroff'});
};

NFCManager.prototype.startPoll = function() {
  return createPromise({'cmd': 'pollstart'});
};

NFCManager.prototype.stopPoll = function() {
  return createPromise({'cmd': 'pollstop'});
};

///////////////////////////////////////////////////////////////////////////////
// NFC - NFCTag
///////////////////////////////////////////////////////////////////////////////

function NFCTag() {}

NFCTag.prototype.readNDEF = function() {
  return createPromise({'cmd': 'readNdef'});
};

// NDEFMessage message
NFCTag.prototype.writeNDEF = function(message) {
  var msg = {
    'cmd': 'writeNdef',
    'message': message
  };
  return createPromise(msg);
};

///////////////////////////////////////////////////////////////////////////////
// NFC - NFCPeer
///////////////////////////////////////////////////////////////////////////////

function NFCPeer() {
  var nfc_peer_listeners = {};
  nfc_peer_listeners['messageread'] = [];
  EventTargetInterface.call(this,
      nfc_peer_listeners, function(type) { return type === 'messageread';});
  this.onmessageread = null;
}
derive(NFCPeer, EventTargetInterface);

// NDEFMessage message
NFCPeer.prototype.sendNDEF = function(message) {
  var msg = {
    'cmd': 'sendNdef',
    'message': message
  };
  return createPromise(msg);
};

// HandoverType handoverType
NFCPeer.prototype.startHandover = function(handoverType) {
  var msg = {
    'cmd': 'startHandover',
    'handoverType': handoverType
  };
  return createPromise(msg);
};

function handleMessageRead(msg) {
  if (connected_peer_ !== null) {
    var ndefRecords = [];
    msg.result.forEach(function(record) {
      ndefRecords.push(toNDEFRecord(record));
    });
    var event = new CustomEvent('messageread');
    var ndefMessage = new NDEFMessage(ndefRecords);
    _addConstProperty(event, 'message', ndefMessage);
    connected_peer_.dispatchEvent(event);
    if (connected_peer_.onmessageread &&
        typeof connected_peer_.onmessageread === 'function')
      connected_peer_.onmessageread(ndefMessage);
  }
}

///////////////////////////////////////////////////////////////////////////////
// NFC - NDEFMessage
///////////////////////////////////////////////////////////////////////////////

function NDEFMessage(data) {
  var records = data;
  if (Array.isArray(data) && data.length > 0 &&
      typeof data[0] === 'number')
    records = [new NDEFRecord(5, null, data, null, 'unknown')];
  _addConstProperty(this, 'records', records);
}

NDEFMessage.prototype.getBytes = function() {
  var msg = {
    'cmd': 'getBytes',
    'records': records
  };
  return createPromise(msg);
};

window.NDEFMessage = NDEFMessage;

///////////////////////////////////////////////////////////////////////////////
// NFC - NDEFRecord
// TNF types:
// 0 (Empty), 1 (Well known), 2 (Media), 3 (URI), 4 (External), 5 (Unknown)
///////////////////////////////////////////////////////////////////////////////

// NDEFRecordType recordType
// byte tnf
// DOMString type
// byte array
// DOMString id

function NDEFRecord(tnf, type, payload, id, recordType) {
  if (typeof recordType === 'undefined')
    recordType = 'unknown';
  _addConstProperty(this, 'recordType', recordType);
  _addConstProperty(this, 'tnf', tnf);
  _addConstProperty(this, 'type', type);
  _addConstProperty(this, 'payload', payload);
  _addConstProperty(this, 'id', id);
}

NDEFRecord.prototype.getPayload = function() {
  var msg = {
    'cmd': 'getPayload',
    'record': this
  };
  return createPromise(msg);
};

window.NDEFRecord = NDEFRecord;

///////////////////////////////////////////////////////////////////////////////
// NFC - NDEFRecordText
///////////////////////////////////////////////////////////////////////////////

function NDEFRecordText(text, languageCode, encoding) {
  NDEFRecord.call(this, 1, 'T', null, null, 'text');
  _addConstProperty(this, 'text', text);
  _addConstProperty(this, 'languageCode', languageCode);
  _addConstProperty(this, 'encoding', encoding);
}

derive(NDEFRecordText, NDEFRecord);
window.NDEFRecordText = NDEFRecordText;

///////////////////////////////////////////////////////////////////////////////
// NFC - NDEFRecordURI
///////////////////////////////////////////////////////////////////////////////

// DOMString uri
function NDEFRecordURI(uri) {
  NDEFRecord.call(this, 1, 'U', null, null, 'uri');
  _addConstProperty(this, 'uri', uri);
}

derive(NDEFRecordURI, NDEFRecord);
window.NDEFRecordURI = NDEFRecordURI;

///////////////////////////////////////////////////////////////////////////////
// NFC - NDEFRecordMedia
///////////////////////////////////////////////////////////////////////////////

// DOMString mimeType
// byte array payload
function NDEFRecordMedia(mimeType, payload) {
  NDEFRecord.call(this, 2, mimeType, payload, null, 'media');
  _addConstProperty(this, 'mimeType', mimeType);
}

derive(NDEFRecordMedia, NDEFRecord);
window.NDEFRecordMedia = NDEFRecordMedia;

///////////////////////////////////////////////////////////////////////////////
// NFC - NDEFRecordSmartPoster
///////////////////////////////////////////////////////////////////////////////

// constructor
// DOMString uri,
// optional NDEFRecordText array titles,
// optional DOMString action,
// optional NDEFRecordMedia array icons,
// optional unsigned long targetSize,
// optional DOMString targetMIME

function NDEFRecordSmartPoster(uri, titles, action,
    icons, targetSize, targetMIME) {
  NDEFRecord.call(this, 3, 'Sp', null, null, 'smartPoster');
  _addConstProperty(this, 'uri', uri);
  _addConstProperty(this, 'titles', titles);
  _addConstProperty(this, 'action', action);
  _addConstProperty(this, 'icons', icons);
  _addConstProperty(this, 'targetSize', targetSize);
  _addConstProperty(this, 'targetMIME', targetMIME);
}

derive(NDEFRecordSmartPoster, NDEFRecord);
window.NDEFRecordSmartPoster = NDEFRecordSmartPoster;

///////////////////////////////////////////////////////////////////////////////
// NFC - NDEFRecordType
///////////////////////////////////////////////////////////////////////////////

var NDEFRecordType = {
  text: 0,
  uri: 1,
  media: 2,
  smartPoster: 3,
  unknown: 4
};

///////////////////////////////////////////////////////////////////////////////
// NFC - SmartPosterAction
///////////////////////////////////////////////////////////////////////////////

var SmartPosterAction = {
  do: 0,
  save: 1,
  open: 2
};

///////////////////////////////////////////////////////////////////////////////
// NFC - HandoverType
///////////////////////////////////////////////////////////////////////////////

var HandoverType = {
  wifi: 0,
  bluetooth: 1
};

///////////////////////////////////////////////////////////////////////////////
// NFCManager - Instance
///////////////////////////////////////////////////////////////////////////////

var g_nfc_manager = new NFCManager();
exports = g_nfc_manager;

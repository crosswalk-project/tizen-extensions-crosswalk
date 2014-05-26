// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var _listener = null;

var postMessage = function(msg) {
  extension.postMessage(JSON.stringify(msg));
};

var sendSyncMessage = function(msg) {
  return extension.internal.sendSyncMessage(JSON.stringify(msg));
};

extension.setMessageListener(function(json) {
  var msg = JSON.parse(json);
  if (msg.cmd == 'FoundCommands') {
    if (_listener) _listener(msg.commands);
    else console.error('[Speech-E]: No listener set!!!');
  } else {
    console.warn('[Speech-W]: Ignorning molformed message: ' + json);
  }
});

exports.vocalizeString = function(speakText) {
  try {
    if (speakText == null || speakText == undefined ||
        typeof speakText !== 'string' || speakText.length == 0) {
      console.warn('[Speech-W]: Ignoring invalid text.');
      return;
    }
    var r = JSON.parse(sendSyncMessage({
      'cmd': 'VocalizeString',
      'text': speakText,
      'language': 'english'
    }));
    if (r.error) {
      console.error('[Speech-E]: Error - ' + r.errorMsg);
    }
  } catch (e) {
    console.error('[Speech-E]: Parsing error: ', e.name);
  }
};

exports.setCBListener = function(listener) {
  try {
    if (listener == undefined) listener = null;

    if (_listener == listener) return;

    if (listener && typeof listener !== 'function') {
      console.error('[Speech-E]: Type mismatch : expected type ' +
          "'funciton' but passed '" + typeof listener + "'");
      return;
    }

    var r = JSON.parse(sendSyncMessage({
      'cmd': 'ListenVoice',
      'listen': (listener != null)
    }));
    if (r.error) {
      console.error('[Speech-E]: Faild to attach Callback Listener');
      return;
    }
    _listener = listener;
  } catch (e) {
    console.error('[Speech-E]: Internal error: ' + e.name);
  }
};

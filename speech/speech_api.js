// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var postMessage = function(msg) {
  extension.postMessage(JSON.stringify(msg));
};

var sendSyncMessage = function(msg) {
  return extension.internal.sendSyncMessage(JSON.stringify(msg));
};

exports.vocalizeString = function(speakText) {
  try {
    var r = JSON.parse(sendSyncMessage({
      'cmd': 'VocalizeString',
      'text': speakText
    }));
  } catch (e) {
    console.error('Parsing error: ', e);
  }
  return r['text'];
};

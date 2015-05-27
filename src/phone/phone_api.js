// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var _callbacks = {};
var _nextReplyId = 0;

var _selected_listeners = {};
var _selected_listener_id = 0;
var _selected_listeners_count = 0;

var _call_changed_listeners = {};
var _call_changed_listener_id = 0;
var _call_changed_listeners_count = 0;

var _call_history_added_listeners = {};
var _call_history_added_listener_id = 0;
var _call_history_added_listeners_count = 0;

var _call_history_changed_listeners = {};
var _call_history_changed_listener_id = 0;
var _call_history_changed_listeners_count = 0;

var _contacts_changed_listeners = {};
var _contacts_changed_listener_id = 0;
var _contacts_changed_listeners_count = 0;

function PhoneError(code, message) {
  Object.defineProperties(this, {
    'code': { writable: false, value: uri, enumerable: true },
    'message': { writable: false, value: id, enumerable: true }
  });
}

function ActiveCall(json) {
  for (var field in json) {
    var val = json[field];
    Object.defineProperty(this, field, { writable: false, value: val, enumerable: true });
  }
}

function getNextReplyId() {
  return _nextReplyId++;
}

function postMessage(msg, callback) {
  var replyId = getNextReplyId();
  _callbacks[replyId] = callback;
  msg.replyId = replyId;
  extension.postMessage(JSON.stringify(msg));
}

var sendSyncMessage = function(msg) {
  return JSON.parse(extension.internal.sendSyncMessage(JSON.stringify(msg)));
};

extension.setMessageListener(function(msg) {
  var m = JSON.parse(msg);
  var replyId = m.replyId;
  var callback = _callbacks[replyId];

  if (m.cmd === 'signal') {
    if (!m.signal_name) {
      console.error('Invalid signal from Phone api');
      return;
    }

    if (m.signal_name === 'RemoteDeviceSelected') {
      handleRemoteDeviceSelectedSignal(m);
    } else if (m.signal_name === 'CallChanged') {
      handleCallChangedSignal(m);
    } else if (m.signal_name === 'CallHistoryEntryAdded') {
      handleCallHistoryEntryAddedSignal(m);
    } else if (m.signal_name === 'CallHistoryChanged') {
      handleCallHistoryChangedSignal(m);
    } else if (m.signal_name === 'ContactsChanged') {
      handleContactsChangedSignal(m);
    }
  } else if (!isNaN(parseInt(replyId)) && (typeof(callback) === 'function')) {
    callback(m);
    delete m.replyId;
    delete _callbacks[replyId];
  } else {
    console.error('Invalid replyId from Phone api: ' + replyId);
  }
});

function handleRemoteDeviceSelectedSignal(msg) {
  for (var key in _selected_listeners) {
    var cb = _selected_listeners[key];
    if (!cb || typeof(cb) !== 'function') {
      console.error('No listener object found for id ' + key);
      throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);
    }
    cb(msg.signal_value);
  }
}

function handleCallChangedSignal(msg) {
  for (var key in _call_changed_listeners) {
    var cb = _call_changed_listeners[key];
    if (!cb || typeof(cb) !== 'function') {
      console.error('No listener object found for id ' + key);
      throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);
    }
    cb(msg.signal_value);
  }
}

function handleCallHistoryEntryAddedSignal(msg) {
  for (var key in _call_history_added_listeners) {
    var cb = _call_history_added_listeners[key];
    if (!cb || typeof(cb) !== 'function') {
      console.error('No listener object found for id ' + key);
      throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);
    }
    cb(msg.signal_value);
  }
}

function handleCallHistoryChangedSignal(msg) {
  for (var key in _call_history_changed_listeners) {
    var cb = _call_history_changed_listeners[key];
    if (!cb || typeof(cb) !== 'function') {
      console.error('No listener object found for id ' + key);
      throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);
    }
    cb();
  }
}

function handleContactsChangedSignal(msg) {
  for (var key in _contacts_changed_listeners) {
    var cb = _contacts_changed_listeners[key];
    if (!cb || typeof(cb) !== 'function') {
      console.error('No listener object found for id ' + key);
      throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);
    }
    cb();
  }
}

exports.selectRemoteDevice = function(address) {
  if (typeof address !== 'string' && address != undefined)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var msg = { cmd: 'SelectRemoteDevice', address: address };
  postMessage(msg, function(result) {
    if (result.isError) {
      console.error('SelectedRemoteDevice failed');
      throw new tizen.WebAPIException(result.errorCode);
    }
  });
};

exports.unselectRemoteDevice = function() {
  var msg = { cmd: 'UnselectRemoteDevice' };
  postMessage(msg, function(result) {
    if (result.isError) {
      console.error('UnselectRemoteDevice failed');
      throw new tizen.WebAPIException(result.errorCode);
    }
  });
};

exports.getSelectedRemoteDevice = function(successCallback) {
  var msg = { cmd: 'GetSelectedRemoteDevice' };
  postMessage(msg, function(result) {
    if (result.isError) {
      console.error('GetSelectedRemoteDevice failed');
      throw new tizen.WebAPIException(result.errorCode);
    }

    if (successCallback && result.value != undefined) {
      successCallback(result.value);
    }
  });
};

exports.invokeCall = function(phoneNumber, errorCallback) {
  if (typeof phoneNumber !== 'string' && phoneNumber != undefined)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var msg = { cmd: 'InvokeCall', phoneNumber: phoneNumber };
  postMessage(msg, function(result) {
    if (result.isError) {
      console.error('InvokeCall failed');
      if (errorCallback) {
        var error = { message: 'InvokeCall failed' };
        if (result.errorMessage)
          error.message += ', error: ' + result.errorMessage;
        errorCallback(error);
      }
    }
  });
};

exports.answerCall = function(errorCallback) {
  var msg = { cmd: 'AnswerCall' };
  postMessage(msg, function(result) {
    if (result.isError) {
      console.error('AnswerCall failed');
      if (errorCallback) {
        var error = { message: 'AnswerCall failed' };
        if (result.errorMessage)
          error.message += ', error: ' + result.errorMessage;
        errorCallback(error);
      }
    }
  });
};

exports.hangupCall = function(errorCallback) {
  var msg = { cmd: 'HangupCall' };
  postMessage(msg, function(result) {
    if (result.isError) {
      console.error('HangupCall failed');
      if (errorCallback) {
        var error = { message: 'HangupCall failed' };
        if (result.errorMessage)
          error.message += ', error: ' + result.errorMessage;
        errorCallback(error);
      }
    }
  });
};

exports.activeCall = function() {
  var result = sendSyncMessage({ cmd: 'ActiveCall' });
  if (result.isError) {
    console.error('activeCall failed');
    throw new tizen.WebAPIException(result.errorCode);
  }

  if (result.value != undefined) {
    return new ActiveCall(result.value);
  }

  return null;
};

exports.muteCall = function(mute, errorCallback) {
  if (typeof mute !== 'boolean' && mute != undefined)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var msg = { cmd: 'MuteCall', mute: mute };
  postMessage(msg, function(result) {
    if (result.isError) {
      console.error('MuteCall failed');
      if (errorCallback) {
        var error = { message: 'MuteCall failed' };
        if (result.errorMessage)
          error.message += ', error: ' + result.errorMessage;
        errorCallback(error);
      }
    }
  });
};

exports.getContacts = function(count, successCallback, errorCallback) {
  if (typeof count !== 'number' && count != undefined)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var msg = { cmd: 'GetContacts', count: count };
  postMessage(msg, function(result) {
    if (result.isError) {
      console.error('GetContacts failed');
      if (errorCallback) {
        var error = { message: 'GetContacts failed' };
        if (result.errorMessage)
          error.message += ', error: ' + result.errorMessage;
        errorCallback(error);
      }
    }

    if (successCallback && result.value != undefined) {
      successCallback(result.value);
    }
  });
};

exports.getCallHistory = function(count, successCallback, errorCallback) {
  if (typeof count !== 'number' && count != undefined)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var msg = { cmd: 'GetCallHistory', count: count };
  postMessage(msg, function(result) {
    if (result.isError) {
      console.error('GetCallHistory failed');
      if (errorCallback) {
        var error = { message: 'GetCallHistory failed' };
        if (result.errorMessage)
          error.message += ', error: ' + result.errorMessage;
        errorCallback(error);
      }
    }

    if (successCallback && result.value != undefined) {
      successCallback(result.value);
    }
  });
};

exports.addRemoteDeviceSelectedListener = function(listener) {
  if (!(listener instanceof Function) && listener != undefined)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  for (var key in _selected_listeners) {
    if (_selected_listeners[key] == listener) {
      console.log('same listener added');
      return key;
    }
  }

  _selected_listeners[++_selected_listener_id] = listener;
  _selected_listeners_count++;
  if (_selected_listeners_count == 1) {
    var msg = { cmd: 'AddRemoteDeviceSelectedListener' };
    postMessage(msg, function(result) {
      if (result.isError) {
        console.error('AddRemoteDeviceSelectedListener failed');
        throw new tizen.WebAPIException(result.errorCode);
      }
    });
  }

  return _selected_listener_id;
};

exports.removeRemoteDeviceSelectedListener = function(listener_id) {
  if (typeof listener_id !== 'number' && listener_id != undefined)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var listener = _selected_listeners[listener_id];
  if (!listener) {
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);
  }

  delete _selected_listeners[listener_id];
  _selected_listeners_count--;
  if (_selected_listeners_count == 0) {
    var msg = { cmd: 'RemoveRemoteDeviceSelectedListener' };
    postMessage(msg, function(result) {
      if (result.isError) {
        console.error('RemoveRemoteDeviceSelectedListener failed');
        throw new tizen.WebAPIException(result.errorCode);
      }
    });
  }
};

exports.addCallChangedListener = function(listener) {
  if (!(listener instanceof Function) && listener != undefined)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  for (var key in _call_changed_listeners) {
    if (_call_changed_listeners[key] == listener) {
      console.log('same listener added');
      return key;
    }
  }

  _call_changed_listeners[++_call_changed_listener_id] = listener;
  _call_changed_listeners_count++;
  if (_call_changed_listeners_count == 1) {
    var msg = { cmd: 'AddCallChangedListener' };
    postMessage(msg, function(result) {
      if (result.isError) {
        console.error('AddCallChangedListener failed');
        throw new tizen.WebAPIException(result.errorCode);
      }
    });
  }

  return _call_changed_listener_id;
};

exports.removeCallChangedListener = function(listener_id) {
  if (typeof listener_id !== 'number' && listener_id != undefined)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var listener = _call_changed_listeners[listener_id];
  if (!listener) {
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);
  }

  delete _call_changed_listeners[listener_id];
  _call_changed_listeners_count--;
  if (_call_changed_listeners_count == 0) {
    var msg = { cmd: 'RemoveCallChangedListener' };
    postMessage(msg, function(result) {
      if (result.isError) {
        console.error('RemoveCallChangedListener failed');
        throw new tizen.WebAPIException(result.errorCode);
      }
    });
  }
};

exports.addCallHistoryEntryAddedListener = function(listener) {
  if (!(listener instanceof Function) && listener != undefined)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  for (var key in _call_history_added_listeners) {
    if (_call_history_added_listeners[key] == listener) {
      console.log('same listener added');
      return key;
    }
  }

  _call_history_added_listeners[++_call_history_added_listener_id] = listener;
  _call_history_added_listeners_count++;
  if (_call_history_added_listeners_count == 1) {
    var msg = { cmd: 'AddCallHistoryEntryAddedListener' };
    postMessage(msg, function(result) {
      if (result.isError) {
        console.error('AddCallHistoryEntryAddedListener failed');
        throw new tizen.WebAPIException(result.errorCode);
      }
    });
  }

  return _call_history_added_listener_id;
};

exports.removeCallHistoryEntryAddedListener = function(listener_id) {
  if (typeof listener_id !== 'number' && listener_id != undefined)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var listener = _call_history_added_listeners[listener_id];
  if (!listener) {
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);
  }

  delete _call_history_added_listeners[listener_id];
  _call_history_added_listeners_count--;
  if (_call_history_added_listeners_count == 0) {
    var msg = { cmd: 'RemoveCallHistoryEntryAddedListener' };
    postMessage(msg, function(result) {
      if (result.isError) {
        console.error('RemoveCallHistoryEntryAddedListener failed');
        throw new tizen.WebAPIException(result.errorCode);
      }
    });
  }
};

exports.addCallHistoryChangedListener = function(listener) {
  if (!(listener instanceof Function) && listener != undefined)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  for (var key in _call_history_changed_listeners) {
    if (_call_history_changed_listeners[key] == listener) {
      console.log('same listener added');
      return key;
    }
  }

  _call_history_changed_listeners[++_call_history_changed_listener_id] = listener;
  _call_history_changed_listeners_count++;
  if (_call_history_changed_listeners_count == 1) {
    var msg = { cmd: 'AddCallHistoryChangedListener' };
    postMessage(msg, function(result) {
      if (result.isError) {
        console.error('AddCallHistoryChangedListener failed');
        throw new tizen.WebAPIException(result.errorCode);
      }
    });
  }

  return _call_history_changed_listener_id;
};

exports.removeCallHistoryChangedListener = function(listener_id) {
  if (typeof listener_id !== 'number' && listener_id != undefined)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var listener = _call_history_changed_listeners[listener_id];
  if (!listener) {
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);
  }

  delete _call_history_changed_listeners[listener_id];
  _call_history_changed_listeners_count--;
  if (_call_history_changed_listeners_count == 0) {
    var msg = { cmd: 'RemoveCallHistoryChangedListener' };
    postMessage(msg, function(result) {
      if (result.isError) {
        console.error('RemoveCallHistoryChangedListener failed');
        throw new tizen.WebAPIException(result.errorCode);
      }
    });
  }
};

exports.addContactsChangedListener = function(listener) {
  if (!(listener instanceof Function) && listener != undefined)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  for (var key in _contacts_changed_listeners) {
    if (_contacts_changed_listeners[key] == listener) {
      console.log('same listener added');
      return key;
    }
  }

  _contacts_changed_listeners[++_contacts_changed_listener_id] = listener;
  _contacts_changed_listeners_count++;
  if (_contacts_changed_listeners_count == 1) {
    var msg = { cmd: 'AddContactsChangedListener' };
    postMessage(msg, function(result) {
      if (result.isError) {
        console.error('AddContactsChangedListener failed');
        throw new tizen.WebAPIException(result.errorCode);
      }
    });
  }

  return _contacts_changed_listener_id;
};

exports.removeContactsChangedListener = function(listener_id) {
  if (typeof listener_id !== 'number' && listener_id != undefined)
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);

  var listener = _contacts_changed_listeners[listener_id];
  if (!listener) {
    throw new tizen.WebAPIException(tizen.WebAPIException.INVALID_VALUES_ERR);
  }

  delete _contacts_changed_listeners[listener_id];
  _contacts_changed_listeners_count--;
  if (_contacts_changed_listeners_count == 0) {
    var msg = { cmd: 'RemoveContactsChangedListener' };
    postMessage(msg, function(result) {
      if (result.isError) {
        console.error('RemoveContactsChangedListener failed');
        throw new tizen.WebAPIException(result.errorCode);
      }
    });
  }
};

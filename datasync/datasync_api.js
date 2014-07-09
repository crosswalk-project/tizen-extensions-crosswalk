// Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// those variables are not exported anyway so no closure
var callbackId = 0;
var callbacks = {};
var hideProtectedProporties = true;

// for the time of serialization 'write-only' and 'read-only' properties
// should be able to run with correct value
function executeUnrestricted(func) {
  try {
    hideProtectedProporties = false;
    func();
  } finally {
    hideProtectedProporties = true;
  }
}

function nextCallbackId() {
  return callbackId++;
}

extension.setMessageListener(function(msg) {
  var m = JSON.parse(msg);

  if (typeof callbacks[m['callback_key']] !== 'object') {
    return;
  }

  var callback = callbacks[m['callback_key']];
  var answer = m['answer'];
  var callbackName = m['callback_name'];

  if (typeof answer['profileId'] !== 'string') {
    return;
  }
  var profileId = answer['profileId'];

  if (callbackName === 'onprogress') {
    if (SyncServiceType.indexOf(answer['serviceType']) === -1 ||
        typeof answer['isFromServer'] !== 'boolean' ||
        typeof answer['totalPerService'] !== 'number' ||
        typeof answer['syncedPerService'] !== 'number') {
      return;
    }

    var serviceType = answer['serviceType'];
    var isFromServer = answer['isFromServer'];
    var totalPerService = answer['totalPerService'];
    var syncedPerService = answer['syncedPerService'];

    callback['onprogress'](profileId, serviceType,
        isFromServer, totalPerService, syncedPerService);
  } else if (callbackName === 'oncompleted') {
    // callback will not be needed anymore
    delete callbacks[m['callback_key']];

    callback['oncompleted'](profileId);
  } else if (callbackName === 'onstopped') {
    // callback will not be needed anymore
    delete callbacks[m['callback_key']];

    callback['onstopped'](profileId);
  } else if (callbackName === 'onfailed') {
    // callback will not be needed anymore
    delete callbacks[m['callback_key']];

    if (typeof answer['error'] !== 'object' ||
        typeof answer['error']['code'] !== 'number' ||
        typeof answer['error']['name'] !== 'string' ||
        typeof answer['error']['message'] !== 'string') {
      return;
    }
    var code = answer['error']['code'];
    var name = answer['error']['name'];
    var message = answer['error']['message'];

    var error = new tizen.WebAPIError(code, name, message);
    callback['onfailed'](profileId, error);
  }
});

function sendSyncMessage(msg) {
  var data = null;
  executeUnrestricted(function() {
    data = JSON.stringify(msg);
  });

  var result = JSON.parse(extension.internal.sendSyncMessage(data));

  if (typeof result !== 'object') {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_UNKNOWN_ERR);
  }

  if (result['exception'] !== undefined) {
    if (typeof result['exception'] !== 'object' ||
        typeof result['exception']['code'] !== 'number' ||
        typeof result['exception']['name'] !== 'string' ||
        typeof result['exception']['message'] !== 'string') {
      throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_UNKNOWN_ERR);
    }

    var code = result['exception']['code'];
    var name = result['exception']['name'];
    var message = result['exception']['message'];

    throw new tizen.WebAPIException(code, name, message);
  } else if (result['answer'] !== undefined) {
    return result['answer'];
  } else {
    return; //undefined
  }
}

function postSyncMessageWithCallback(msg, callback) {
  if (callback) {
    var id = nextCallbackId();
    msg['callback_key'] = id;
    callbacks[id] = callback;
  }
  return sendSyncMessage(msg);
}

function defineReadWriteProperty(object, key, value) {
  Object.defineProperty(object, key, {
    enumerable: true,
    writable: true,
    value: value
  });
}

function defineReadWriteNonNullProperty(object, key, value) {
  var hvalue = value;
  Object.defineProperty(object, key, {
    enumerable: true,
    set: function(val) {
      if (val !== null && val !== undefined) {
        hvalue = val;
      }
    },
    get: function() {
      return hvalue;
    }
  });
}

function defineReadWriteWithAcceptListProperty(object, key, value, acceptList, doThrow) {
  var hvalue = value;
  Object.defineProperty(object, key, {
    enumerable: true,
    set: function(val) {
      if (acceptList.indexOf(val) !== -1) {
        hvalue = val;
      } else {
        if (doThrow) {
          throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
        }
      }
    },
    get: function() {
      return hvalue;
    }
  });
}

function defineReadOnlyProperty(object, key, value) {
  var hvalue = value;
  Object.defineProperty(object, key, {
    enumerable: true,
    set: function(val) {
      if (!hideProtectedProporties) {
        hvalue = val;
      }
    },
    get: function() {
      return hvalue;
    }
  });
}

function defineWriteOnlyProperty(object, key, value, accessedValue) {
  var hvalue = value;
  Object.defineProperty(object, key, {
    enumerable: true,
    set: function(val) {
      hvalue = val;
    },
    get: function() {
      if (hideProtectedProporties) {
        return accessedValue;
      } else {
        return hvalue;
      }
    }
  });
}

// enums
var SyncModes = ['MANUAL', 'PERIODIC', 'PUSH'];
var SyncType = ['TWO_WAY', 'SLOW', 'ONE_WAY_FROM_CLIENT', 'REFRESH_FROM_CLIENT',
                'ONE_WAY_FROM_SERVER', 'REFRESH_FROM_SERVER'];
var SyncInterval = ['5_MINUTES', '15_MINUTES', '1_HOUR', '4_HOURS',
                    '12_HOURS', '1_DAY', '1_WEEK', '1_MONTH'];
var SyncServiceType = ['CONTACT', 'EVENT'];
var SyncStatus = ['SUCCESS', 'FAIL', 'STOP', 'NONE'];

// SyncInfo interface
tizen.SyncInfo = function(url, id, password, mode, typeorinterval) {
  if (!(this instanceof tizen.SyncInfo)) {
    throw new TypeError;
  }

  if (typeof url !== 'string' ||
      typeof id !== 'string' ||
      typeof password !== 'string' ||
      typeof mode !== 'string' ||
      SyncModes.indexOf(mode) === -1) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  defineReadWriteNonNullProperty(this, 'url', url);
  defineWriteOnlyProperty(this, 'id', id, null);
  defineWriteOnlyProperty(this, 'password', password, null);
  defineReadWriteWithAcceptListProperty(this, 'mode', mode, SyncModes, false);

  if (mode === 'MANUAL' && SyncType.indexOf(typeorinterval) !== -1) {
    defineReadWriteWithAcceptListProperty(
        this, 'type', typeorinterval, SyncType.concat([null]), false
    );
    defineReadWriteWithAcceptListProperty(
        this, 'interval', null, SyncInterval.concat([null]), false
    );
  } else if (mode === 'PERIODIC' && SyncInterval.indexOf(typeorinterval) !== -1) {
    defineReadWriteWithAcceptListProperty(
        this, 'type', null, SyncType.concat([null]), false
    );
    defineReadWriteWithAcceptListProperty(
        this, 'interval', typeorinterval, SyncInterval.concat([null]), false
    );
  } else {
    defineReadWriteWithAcceptListProperty(
        this, 'type', null, SyncType.concat([null]), true
    );
    defineReadWriteWithAcceptListProperty(
        this, 'interval', null, SyncInterval.concat([null]), false
    );
  }
};

// SyncServiceInfo interface
tizen.SyncServiceInfo = function(enable, serviceType, serverDatabaseUri, id, password) {
  if (!(this instanceof tizen.SyncServiceInfo)) {
    throw new TypeError;
  }

  if (typeof enable !== 'boolean') {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }
  if (SyncServiceType.indexOf(serviceType) === -1) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }
  if (typeof serverDatabaseUri !== 'string') {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  defineReadWriteNonNullProperty(this, 'enable', enable);
  defineReadWriteWithAcceptListProperty(this, 'serviceType', serviceType, SyncServiceType, false);
  defineReadWriteNonNullProperty(this, 'serverDatabaseUri', serverDatabaseUri);

  if (id !== undefined) {
    if (typeof id !== 'string') {
      throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
    } else {
      defineWriteOnlyProperty(this, 'id', id, null);
    }
  } else {
    defineWriteOnlyProperty(this, 'id', null, null);
  }

  if (password !== undefined) {
    if (typeof password !== 'string') {
      throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
    } else {
      defineWriteOnlyProperty(this, 'password', password, null);
    }
  } else {
    defineWriteOnlyProperty(this, 'password', null, null);
  }
};

// SyncProfileInfo interface
tizen.SyncProfileInfo = function(profileName, syncInfo, serviceInfo) {
  if (!(this instanceof tizen.SyncProfileInfo)) {
    throw new TypeError;
  }

  if (typeof profileName !== 'string') {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }
  if (!(syncInfo instanceof tizen.SyncInfo)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  defineReadOnlyProperty(this, 'profileId', null);
  defineReadWriteNonNullProperty(this, 'profileName', profileName);
  defineReadWriteNonNullProperty(this, 'syncInfo', syncInfo);

  if (serviceInfo !== undefined) {
    if (serviceInfo instanceof Array) {
      for (var i = 0; i < serviceInfo.length; i++) {
        if (!(serviceInfo[i] instanceof tizen.SyncServiceInfo)) {
          throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
        }
      }
      defineReadWriteProperty(this, 'serviceInfo', serviceInfo);
    } else if (serviceInfo === null) {
      defineReadWriteProperty(this, 'serviceInfo', null);
    } else {
      throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
    }
  } else {
    defineReadWriteProperty(this, 'serviceInfo', null);
  }
};

// SyncStatistics interface.
function SyncStatistics(json) {
  for (var field in json) {
    var val = json[field];
    if (val !== undefined) {
      if (field === 'lastSyncTime') {
        val = new Date(val * 1000);
      }
    }
    defineReadOnlyProperty(this, field, val);
  }
}

function validateCallbacks(callbacks) {
  var callbackNames = ['onprogress', 'oncompleted', 'onstopped', 'onfailed'];

  for (var i = 0; i < callbackNames.length; i++) {
    if (typeof callbacks[callbackNames[i]] !== 'function') {
      return false;
    }
  }

  return true;
}

function convertToSyncStatistics(data) {
  if (!(data instanceof Array)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_UNKNOWN_ERR);
  }
  var result = [];
  for (var i = 0; i < data.length; i++) {
    result.push(new SyncStatistics(data[i]));
  }
  return result;
}

function convertToProfileInfo(data) {
  if (typeof data !== 'object') {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_UNKNOWN_ERR);
  }

  var profileId = data['profileId'];
  var profileName = data['profileName'];
  var syncInfo = data['syncInfo'];
  var serviceInfo = data['serviceInfo'];

  if (typeof profileId !== 'string') {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_UNKNOWN_ERR);
  }
  if (typeof profileName !== 'string') {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_UNKNOWN_ERR);
  }
  if (syncInfo === undefined) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_UNKNOWN_ERR);
  }

  var serviceInfoList = [];
  if (serviceInfo instanceof Array) {
    for (var i = 0; i < serviceInfo.length; i++) {
      serviceInfoList.push(convertToServiceInfo(serviceInfo[i]));
    }
  } else if (serviceInfo === null) {
    serviceInfoList = null;
  } else {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_UNKNOWN_ERR);
  }

  var profile = new tizen.SyncProfileInfo(profileName,
      convertToSyncInfo(syncInfo), serviceInfoList);

  executeUnrestricted(function() {
    profile.profileId = profileId;
  });

  return profile;
}

function convertToSyncInfo(data) {
  try {
    var mode = data['mode'];
    var type = data['type'];
    var interval = data['interval'];

    var typeorinterval = null;
    if (mode === 'MANUAL') {
      typeorinterval = type;
    } else if (mode === 'PERIODIC') {
      typeorinterval = interval;
    }

    return new tizen.SyncInfo(data['url'], data['id'], data['password'],
        mode, typeorinterval);
  } catch (e) {
    if (e instanceof tizen.WebAPIException) {
      throw tizen.WebAPIException(tizen.WebAPIException.TYPE_UNKNOWN_ERR);
    } else {
      throw e;
    }
  }
}

function convertToServiceInfo(data) {
  try {
    return new tizen.SyncServiceInfo(data['enable'], data['serviceType'],
        data['serverDatabaseUri'], data['id'], data['password']);
  } catch (e) {
    if (e instanceof tizen.WebAPIException) {
      throw tizen.WebAPIException(tizen.WebAPIException.TYPE_UNKNOWN_ERR);
    } else {
      throw e;
    }
  }
}

// DataSynchronizationManager interface
function DataSynchronizationManager() {
  if (!(this instanceof DataSynchronizationManager)) {
    throw new TypeError;
  }
}

DataSynchronizationManager.prototype.add = function(profile) {
  if (!(profile instanceof tizen.SyncProfileInfo)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }
  var msg = {
    cmd: 'add',
    arg: profile
  };

  var id = sendSyncMessage(msg);
  executeUnrestricted(function() {
    profile.profileId = id;
  });
};

DataSynchronizationManager.prototype.update = function(profile) {
  if (!(profile instanceof tizen.SyncProfileInfo)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }
  var msg = {
    cmd: 'update',
    arg: profile
  };
  return sendSyncMessage(msg);
};

DataSynchronizationManager.prototype.remove = function(profileId) {
  if (typeof profileId !== 'string') {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }
  var msg = {
    cmd: 'remove',
    arg: profileId
  };
  return sendSyncMessage(msg);
};

DataSynchronizationManager.prototype.getMaxProfilesNum = function() {
  var msg = {
    cmd: 'getMaxProfilesNum'
  };
  return sendSyncMessage(msg);
};

DataSynchronizationManager.prototype.getProfilesNum = function() {
  var msg = {
    cmd: 'getProfilesNum'
  };
  return sendSyncMessage(msg);
};

DataSynchronizationManager.prototype.get = function(profileId) {
  if (typeof profileId !== 'string') {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }
  var msg = {
    cmd: 'get',
    arg: profileId
  };

  return convertToProfileInfo(sendSyncMessage(msg));
};

DataSynchronizationManager.prototype.getAll = function() {
  var msg = {
    cmd: 'getAll'
  };

  var result = sendSyncMessage(msg);
  if (!(result instanceof Array)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_UNKNOWN_ERR);
  }
  var final = [];
  for (var i = 0; i < result.length; i++) {
    final.push(convertToProfileInfo(result[i]));
  }
  return final;
};

DataSynchronizationManager.prototype.startSync = function(profileId, progressCallback) {
  if (typeof profileId !== 'string') {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }
  if (arguments.length > 1) {
    // Array is an object, should not accept Array
    if (progressCallback instanceof Array) {
      throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
    }
    if (typeof progressCallback !== 'object') {
      throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
    }
    if (!validateCallbacks(progressCallback)) {
      throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
    }
  }

  var msg = {
    cmd: 'startSync',
    arg: profileId
  };

  return postSyncMessageWithCallback(msg, progressCallback);
};

DataSynchronizationManager.prototype.stopSync = function(profileId) {
  if (typeof profileId !== 'string') {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }
  var msg = {
    cmd: 'stopSync',
    arg: profileId
  };
  return sendSyncMessage(msg);
};

DataSynchronizationManager.prototype.getLastSyncStatistics = function(profileId) {
  if (typeof profileId !== 'string') {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }
  var msg = {
    cmd: 'getLastSyncStatistics',
    arg: profileId
  };
  var result = sendSyncMessage(msg);
  return convertToSyncStatistics(result);
};

exports = new DataSynchronizationManager();

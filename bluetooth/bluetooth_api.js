// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var _callbacks = {};
var _next_reply_id = 0;

var postMessage = function(msg, callback) {
  var reply_id = _next_reply_id;
  _next_reply_id += 1;
  _callbacks[reply_id] = callback;
  msg.reply_id = reply_id.toString();
  extension.postMessage(JSON.stringify(msg));
};

extension.setMessageListener(function(json) {
  var msg = JSON.parse(json);

  if (msg.cmd == "DeviceFound")
    handleDeviceFound(msg);
  else if (msg.cmd == "DiscoveryFinished")
    handleDiscoveryFinished();
  else if (msg.cmd == "DeviceRemoved")
    handleDeviceRemoved(msg.Address);
  else if (msg.cmd == "DeviceUpdated")
    handleDeviceUpdated(msg);
  else if (msg.cmd == "AdapterUpdated")
    handleAdapterUpdated(msg);
  else if (msg.cmd == "RFCOMMSocketAccept")
    handleRFCOMMSocketAccept(msg);
  else { // Then we are dealing with postMessage return.
    var reply_id = msg.reply_id;
    var callback = _callbacks[reply_id];
    if (callback) {
      delete msg.reply_id;
      delete _callbacks[reply_id];
      callback(msg);
    } else {
      console.log('Invalid reply_id from Tizen Bluetooth: ' + reply_id);
    }
  }
});

function Adapter() {
  this.found_devices = []; // Filled while a Discovering.
  this.known_devices = []; // Keeps Managed and Found devices.
  this.discovery_callbacks = {};
  this.isReady = false;
  this.service_handlers = [];
  this.sockets = [];
};

Adapter.prototype.serviceNotAvailable = function(errorCallback) {
  if (adapter.isReady && defaultAdapter.powered)
    return false;

  if (errorCallback) {
    var error = new tizen.WebAPIError(tizen.WebAPIException.SERVICE_NOT_AVAILABLE_ERR);
    errorCallback(error);
  }
  return true;
}

Adapter.prototype.indexOfDevice = function(devices, address) {
  for (var i = 0; i < devices.length; i++) {
    if (devices[i].address == address)
      return i;
  }
  return -1;
};

Adapter.prototype.addDevice = function(device, on_discovery) {
  var new_device = false;

  if (on_discovery) {
    var index = this.indexOfDevice(this.found_devices, device.address);
    if (index == -1) {
      this.found_devices.push(device);
      new_device = true;
    } else {
      this.found_devices[index] = device;
      new_device = false;
    }
  }

  var i = this.indexOfDevice(this.known_devices, device.address);
  if (i == -1)
    this.known_devices.push(device);
  else
    this.known_devices[i] = device;

  return new_device;
};

Adapter.prototype.updateDevice = function(device) {
  var index = this.indexOfDevice(this.known_devices, device.address);
  if (index == -1)
    this.known_devices.push(device);
  else
    this.known_devices[index]._updateProperties(device);
};

// This holds the adapter the Bluetooth backend is currently using.
// In BlueZ 4, for instance, this would represent the "default adapter".
// BlueZ 5 has no such concept, so this will hold the currently available
// adapter, which can be just the first one found.
var adapter = new Adapter();

var deepCopyDevices = function(devices) {
  var copiedDevices = [];
  for (var i = 0; i < devices.length; i++)
    copiedDevices[i] = devices[i]._clone();

  return copiedDevices;
};

var handleDeviceFound = function(msg) {
  var device = new BluetoothDevice(msg);
  var is_new = adapter.addDevice(device, msg.found_on_discovery);

  // FIXME(jeez): we are not returning a deep copy so we can keep
  // the devices up-to-date. We have to find a better way to handle this.
  if (is_new && msg.found_on_discovery)
    adapter.discovery_callbacks.ondevicefound(device);
};

var handleDiscoveryFinished = function() {
  // FIXME(jeez): we are not returning a deep copy so we can keep
  // the devices up-to-date. We have to find a better way to handle this.
  adapter.discovery_callbacks.onfinished(adapter.found_devices);

  adapter.found_devices = [];
  adapter.discovery_callbacks = {};
};

var handleDeviceRemoved = function(address) {
  var foundDevices = adapter.found_devices;
  var knownDevices = adapter.known_devices;

  for (var i = 0; i < foundDevices.length; i++) {
    if (foundDevices[i].address === address) {
      foundDevices.splice(i, 1);
      break;
    }
  }

  for (var i = 0; i < knownDevices.length; i++) {
    if (knownDevices[i].address === address) {
      knownDevices.splice(i, 1);
      break;
    }
  }
};

var handleDeviceUpdated = function(msg) {
  var device = new BluetoothDevice(msg);
  adapter.updateDevice(device);
};

var handleAdapterUpdated = function(msg) {
    if (msg.Name)
      _addConstProperty(defaultAdapter, "name", msg.Name);
    if (msg.Address)
      _addConstProperty(defaultAdapter, "address", msg.Address);
    if (msg.Powered) {
      _addConstProperty(defaultAdapter, "powered",
          (msg.Powered == "true") ? true : false);
    }
    if (msg.Discoverable) {
      _addConstProperty(defaultAdapter, "visible",
          (msg.Discoverable == "true") ? true : false);
    }
};

var handleRFCOMMSocketAccept = function(msg) {
  for (var i in adapter.service_handlers) {
    var server = adapter.service_handlers[i];
    if (server.channel === msg.channel) {
      var j = adapter.indexOfDevice(adapter.known_devices, msg.peer);
      var peer = adapter.known_devices[j];

      var socket = new BluetoothSocket(server.uuid, peer, msg);

      adapter.sockets.push(socket);

      _addConstProperty(server, "isConnected", true);

      if (server.onconnect && typeof server.onconnect === 'function')
        server.onconnect(socket);
      return;
    }
  }
};

var defaultAdapter = new BluetoothAdapter();

exports.getDefaultAdapter = function() {
  var msg = {
    'cmd': 'GetDefaultAdapter'
  };
  var result = JSON.parse(extension.internal.sendSyncMessage(JSON.stringify(msg)));

  if (!result.error) {
    _addConstProperty(defaultAdapter, "name", result.name);
    _addConstProperty(defaultAdapter, "address", result.address);
    _addConstProperty(defaultAdapter, "powered", result.powered);
    _addConstProperty(defaultAdapter, "visible", result.visible);

    if (result.hasOwnProperty("address") && result.address != "")
      adapter.isReady = true;
  } else {
    adapter.isReady = false;
    throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);
  }

  return defaultAdapter;
};

exports.deviceMajor = {};
var deviceMajor = {
  "MISC": { value: 0x00, configurable: false, writable: false },
  "COMPUTER": { value: 0x01, configurable: false, writable: false },
  "PHONE": { value: 0x02, configurable: false, writable: false },
  "NETWORK": { value: 0x03, configurable: false, writable: false },
  "AUDIO_VIDEO": { value: 0x04, configurable: false, writable: false },
  "PERIPHERAL": { value: 0x05, configurable: false, writable: false },
  "IMAGING": { value: 0x06, configurable: false, writable: false },
  "WEARABLE": { value: 0x07, configurable: false, writable: false },
  "TOY": { value: 0x08, configurable: false, writable: false },
  "HEALTH": { value: 0x09, configurable: false, writable: false },
  "UNCATEGORIZED": { value: 0x1F, configurable: false, writable: false }
};
Object.defineProperties(exports.deviceMajor, deviceMajor);

exports.deviceMinor = {};
var deviceMinor = {
  "COMPUTER_UNCATEGORIZED": { value: 0x00, configurable: false, writable: false },
  "COMPUTER_DESKTOP": { value: 0x01, configurable: false, writable: false },
  "COMPUTER_SERVER": { value: 0x02, configurable: false, writable: false },
  "COMPUTER_LAPTOP": { value: 0x03, configurable: false, writable: false },
  "COMPUTER_HANDHELD_PC_OR_PDA": { value: 0x04, configurable: false, writable: false },
  "COMPUTER_PALM_PC_OR_PDA": { value: 0x05, configurable: false, writable: false },
  "COMPUTER_WEARABLE": { value: 0x06, configurable: false, writable: false },
  "PHONE_UNCATEGORIZED": { value: 0x00, configurable: false, writable: false },
  "PHONE_CELLULAR": { value: 0x01, configurable: false, writable: false },
  "PHONE_CORDLESS": { value: 0x02, configurable: false, writable: false },
  "PHONE_SMARTPHONE": { value: 0x03, configurable: false, writable: false },
  "PHONE_MODEM_OR_GATEWAY": { value: 0x04, configurable: false, writable: false },
  "PHONE_ISDN": { value: 0x05, configurable: false, writable: false },
  "AV_UNRECOGNIZED": { value: 0x00, configurable: false, writable: false },
  "AV_WEARABLE_HEADSET": { value: 0x01, configurable: false, writable: false },
  "AV_HANDSFREE": { value: 0x02, configurable: false, writable: false },
  "AV_MICROPHONE": { value: 0x04, configurable: false, writable: false },
  "AV_LOUDSPEAKER": { value: 0x05, configurable: false, writable: false },
  "AV_HEADPHONES": { value: 0x06, configurable: false, writable: false },
  "AV_PORTABLE_AUDIO": { value: 0x07, configurable: false, writable: false },
  "AV_CAR_AUDIO": { value: 0x08, configurable: false, writable: false },
  "AV_SETTOP_BOX": { value: 0x09, configurable: false, writable: false },
  "AV_HIFI": { value: 0x0a, configurable: false, writable: false },
  "AV_VCR": { value: 0x0b, configurable: false, writable: false },
  "AV_VIDEO_CAMERA": { value: 0x0c, configurable: false, writable: false },
  "AV_CAMCORDER": { value: 0x0d, configurable: false, writable: false },
  "AV_MONITOR": { value: 0x0e, configurable: false, writable: false },
  "AV_DISPLAY_AND_LOUDSPEAKER": { value: 0x0f, configurable: false, writable: false },
  "AV_VIDEO_CONFERENCING": { value: 0x10, configurable: false, writable: false },
  "AV_GAMING_TOY": { value: 0x12, configurable: false, writable: false },
  "PERIPHERAL_UNCATEGORIZED": { value: 0, configurable: false, writable: false },
  "PERIPHERAL_KEYBOARD": { value: 0x10, configurable: false, writable: false },
  "PERIPHERAL_POINTING_DEVICE": { value: 0x20, configurable: false, writable: false },
  "PERIPHERAL_KEYBOARD_AND_POINTING_DEVICE": { value: 0x30, configurable: false, writable: false },
  "PERIPHERAL_JOYSTICK": { value: 0x01, configurable: false, writable: false },
  "PERIPHERAL_GAMEPAD": { value: 0x02, configurable: false, writable: false },
  "PERIPHERAL_REMOTE_CONTROL": { value: 0x03, configurable: false, writable: false },
  "PERIPHERAL_SENSING_DEVICE": { value: 0x04, configurable: false, writable: false },
  "PERIPHERAL_DEGITIZER_TABLET": { value: 0x05, configurable: false, writable: false },
  "PERIPHERAL_CARD_READER": { value: 0x06, configurable: false, writable: false },
  "PERIPHERAL_DIGITAL_PEN": { value: 0x07, configurable: false, writable: false },
  "PERIPHERAL_HANDHELD_SCANNER": { value: 0x08, configurable: false, writable: false },
  "PERIPHERAL_HANDHELD_INPUT_DEVICE": { value: 0x09, configurable: false, writable: false },
  "IMAGING_UNCATEGORIZED": { value: 0x00, configurable: false, writable: false },
  "IMAGING_DISPLAY": { value: 0x04, configurable: false, writable: false },
  "IMAGING_CAMERA": { value: 0x08, configurable: false, writable: false },
  "IMAGING_SCANNER": { value: 0x10, configurable: false, writable: false },
  "IMAGING_PRINTER": { value: 0x20, configurable: false, writable: false },
  "WEARABLE_WRITST_WATCH": { value: 0x01, configurable: false, writable: false },
  "WEARABLE_PAGER": { value: 0x02, configurable: false, writable: false },
  "WEARABLE_JACKET": { value: 0x03, configurable: false, writable: false },
  "WEARABLE_HELMET": { value: 0x04, configurable: false, writable: false },
  "WEARABLE_GLASSES": { value: 0x05, configurable: false, writable: false },
  "TOY_ROBOT": { value: 0x01, configurable: false, writable: false },
  "TOY_VEHICLE": { value: 0x02, configurable: false, writable: false },
  "TOY_DOLL": { value: 0x03, configurable: false, writable: false },
  "TOY_CONTROLLER": { value: 0x04, configurable: false, writable: false },
  "TOY_GAME": { value: 0x05, configurable: false, writable: false },
  "HEALTH_UNDEFINED": { value: 0x00, configurable: false, writable: false },
  "HEALTH_BLOOD_PRESSURE_MONITOR": { value: 0x01, configurable: false, writable: false },
  "HEALTH_THERMOMETER": { value: 0x02, configurable: false, writable: false },
  "HEALTH_WEIGHING_SCALE": { value: 0x03, configurable: false, writable: false },
  "HEALTH_GLUCOSE_METER": { value: 0x04, configurable: false, writable: false },
  "HEALTH_PULSE_OXIMETER": { value: 0x05, configurable: false, writable: false },
  "HEALTH_PULSE_RATE_MONITOR": { value: 0x06, configurable: false, writable: false },
  "HEALTH_DATA_DISPLAY": { value: 0x07, configurable: false, writable: false },
  "HEALTH_STEP_COUNTER": { value: 0x08, configurable: false, writable: false },
  "HEALTH_BODY_COMPOSITION_ANALYZER": { value: 0x09, configurable: false, writable: false },
  "HEALTH_PEAK_FLOW_MONITOR": { value: 0x0a, configurable: false, writable: false },
  "HEALTH_MEDICATION_MONITOR": { value: 0x0b, configurable: false, writable: false },
  "HEALTH_KNEE_PROSTHESIS": { value: 0x0c, configurable: false, writable: false },
  "HEALTH_ANKLE_PROSTHESIS": { value: 0x0d, configurable: false, writable: false }
};
Object.defineProperties(exports.deviceMinor, deviceMinor);

exports.deviceService = {};
var deviceService = {
  "LIMITED_DISCOVERABILITY": { value: 0x0001, configurable: false, writable: false },
  "POSITIONING": { value: 0x0008, configurable: false, writable: false },
  "NETWORKING": { value: 0x0010, configurable: false, writable: false },
  "RENDERING": { value: 0x0020, configurable: false, writable: false },
  "CAPTURING": { value: 0x0040, configurable: false, writable: false },
  "OBJECT_TRANSFER": { value: 0x0080, configurable: false, writable: false },
  "AUDIO": { value: 0x0100, configurable: false, writable: false },
  "TELEPHONY": { value: 0x0200, configurable: false, writable: false },
  "INFORMATION": { value: 0x0400, configurable: false, writable: false }
};
Object.defineProperties(exports.deviceService, deviceService);

function _addConstProperty(obj, propertyKey, propertyValue) {
  Object.defineProperty(obj, propertyKey, {
    configurable: true,
    writable: false,
    value: propertyValue
  });
};

function BluetoothAdapter() {
  _addConstProperty(this, "name", "");
  _addConstProperty(this, "address", "00:00:00:00:00:00");
  _addConstProperty(this, "powered", false);
  _addConstProperty(this, "visible", false);
};

BluetoothAdapter.prototype.setName = function(name, successCallback, errorCallback) {
  if (adapter.serviceNotAvailable(errorCallback))
    return;

  if (typeof name !== 'string'
        || (successCallback && typeof successCallback !== 'function')
        || (errorCallback && typeof errorCallback !== 'function')) {
    if (errorCallback) {
      var error = new tizen.WebAPIError(tizen.WebAPIException.INVALID_VALUES_ERR);
      errorCallback(error);
    }

    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
    return;
  }

  var msg = {
    'cmd': 'SetAdapterProperty',
    'property': 'Name',
    'value': name
  };

  postMessage(msg, function(result) {
    if (result.error != 0) {
      if (errorCallback) {
        var error = new tizen.WebAPIError(tizen.WebAPIException.INVALID_VALUES_ERR);
        errorCallback(error);
      }

      throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
      return;
    }

    if (successCallback)
      successCallback();
  });
};

BluetoothAdapter.prototype.setPowered = function(state, successCallback, errorCallback) {
  throw new tizen.WebAPIException(tizen.WebAPIException.NOT_SUPPORTED_ERR);
};

BluetoothAdapter.prototype.setVisible = function(mode, successCallback, errorCallback, timeout) {
  if (adapter.serviceNotAvailable(errorCallback))
    return;

  if (typeof mode !== 'boolean'
        || (successCallback && typeof successCallback !== 'function')
        || (errorCallback && typeof errorCallback !== 'function')) {
    if (errorCallback) {
      var error = new tizen.WebAPIError(tizen.WebAPIException.INVALID_VALUES_ERR);
      errorCallback(error);
    }

    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
    return;
  }

  if (timeout === undefined || typeof timeout !== 'number' || timeout < 0)
    timeout = 180; // According to tizen.bluetooth documentation.

  var msg = {
    'cmd': 'SetAdapterProperty',
    'property': 'Discoverable',
    'value': mode,
    'timeout': timeout
  };

  postMessage(msg, function(result) {
    if (result.error != 0) {
      if (errorCallback) {
        var error = new tizen.WebAPIError(tizen.WebAPIException.INVALID_VALUES_ERR);
        errorCallback(error);
      }

      throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
      return;
    }

    if (successCallback)
      successCallback();
  });
};

BluetoothAdapter.prototype.discoverDevices = function(discoverySuccessCallback, errorCallback) {
  if (adapter.serviceNotAvailable(errorCallback))
    return;

  if ((discoverySuccessCallback && typeof discoverySuccessCallback !== 'object')
        || (errorCallback && typeof errorCallback !== 'function')) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  var msg = {
    'cmd': 'DiscoverDevices'
  };
  postMessage(msg, function(result) {
    if (result.error != 0) {
      if (errorCallback) {
        var error = new tizen.WebAPIError(tizen.WebAPIException.UNKNOWN_ERR);
        errorCallback(error);
      }
      return;
    }

    adapter.discovery_callbacks = discoverySuccessCallback;
    discoverySuccessCallback.onstarted();
  });
};

BluetoothAdapter.prototype.stopDiscovery = function(successCallback, errorCallback) {
  if (adapter.serviceNotAvailable(errorCallback))
    return;

  if ((successCallback && typeof successCallback !== 'function')
        || (errorCallback && typeof errorCallback !== 'function')) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  var msg = {
    'cmd': 'StopDiscovery'
  };
  postMessage(msg, function(result) {
    if (result.error != 0) {
      if (errorCallback) {
        var error = new tizen.WebAPIError(tizen.WebAPIException.UNKNOWN_ERR);
        errorCallback(error);
      }
      return;
    }

    successCallback();
    handleDiscoveryFinished();
  });
};

BluetoothAdapter.prototype.getKnownDevices = function(deviceArraySuccessCallback, errorCallback) {
  if (adapter.serviceNotAvailable(errorCallback))
    return;

  if ((deviceArraySuccessCallback && typeof deviceArraySuccessCallback !== 'function')
        || (errorCallback && typeof errorCallback !== 'function')) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  // FIXME(jeez): we are not returning a deep copy so we can keep
  // the devices up-to-date. We have to find a better way to handle this.
  deviceArraySuccessCallback(adapter.known_devices);
};

BluetoothAdapter.prototype.getDevice = function(address, deviceSuccessCallback, errorCallback) {
  if (adapter.serviceNotAvailable(errorCallback))
    return;

  if ((deviceSuccessCallback && typeof deviceSuccessCallback !== 'function') ||
      (errorCallback && typeof errorCallback !== 'function')) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  if (typeof address !== 'string') {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  var index = adapter.indexOfDevice(adapter.known_devices, address)
  if (index == -1) {
    var error = new tizen.WebAPIError(tizen.WebAPIException.NOT_FOUND_ERR)
    errorCallback(error)
    return;
  }

  deviceSuccessCallback(adapter.known_devices[index])
};

BluetoothAdapter.prototype.createBonding = function(address, successCallback, errorCallback) {
  if (adapter.serviceNotAvailable(errorCallback))
    return;

  if (typeof address !== 'string'
        || (successCallback && typeof successCallback !== 'function')
        || (errorCallback && typeof errorCallback !== 'function')) {
    if (errorCallback) {
      var error = new tizen.WebAPIError(tizen.WebAPIException.TYPE_MISMATCH_ERR);
      errorCallback(error);
    }
  }

  var msg = {
    'cmd': 'CreateBonding',
    'address': address
  };

  postMessage(msg, function(result) {
    var cb_device;

    if (result.error != 0) {
      if (errorCallback) {
        var error = new tizen.WebAPIError(tizen.WebAPIException.INVALID_VALUES_ERR);
        errorCallback(error);
      }

      throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
      return;
    }

    if (successCallback) {
      var known_devices = adapter.known_devices;
      for (var i = 0; i < known_devices.length; i++) {
        if (known_devices[i].address === address) {
          cb_device = known_devices[i];
          break;
        }
      }

     successCallback(cb_device);
   }
  });
};

BluetoothAdapter.prototype.destroyBonding = function(address, successCallback, errorCallback) {
  if (adapter.serviceNotAvailable(errorCallback))
    return;

  if (typeof address !== 'string'
        || (successCallback && typeof successCallback !== 'function')
        || (errorCallback && typeof errorCallback !== 'function')) {
    if (errorCallback) {
      var error = new tizen.WebAPIError(tizen.WebAPIException.TYPE_MISMATCH_ERR);
      errorCallback(error);
    }
  }

  var msg = {
    'cmd': 'DestroyBonding',
    'address': address
  };

  postMessage(msg, function(result) {
    var cb_device;

    if (result.error != 0) {
      if (errorCallback) {
        var error_type = (result.error == 1) ? tizen.WebAPIException.NOT_FOUND_ERR : tizen.WebAPIException.UNKNOWN_ERR;
        var error = new tizen.WebAPIError(error_type);
        errorCallback(error);
      }

      throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
      return;
    }

    if (successCallback) {
      var known_devices = adapter.known_devices;
      for (var i = 0; i < known_devices.length; i++) {
        if (known_devices[i].address === address) {
          cb_device = known_devices[i];
          break;
        }
      }

     successCallback(cb_device);
   }
  });
};

BluetoothAdapter.prototype.registerRFCOMMServiceByUUID = function(uuid, name, serviceSuccessCallback, errorCallback) {
  if (adapter.serviceNotAvailable(errorCallback))
    return;

  if (typeof uuid !== 'string'
      || typeof name !== 'string'
      || (serviceSuccessCallback && typeof serviceSuccessCallback !== 'function')
      || (errorCallback && typeof errorCallback !== 'function')) {
    if (errorCallback) {
      var error = new tizen.WebAPIError(tizen.WebAPIException.TYPE_MISMATCH_ERR);
      errorCallback(error);
    }
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
    return;
  }

  var msg = {
    'cmd': 'RFCOMMListen',
    'uuid': uuid,
    'name': name
  };

  postMessage(msg, function(result) {
    if (result.error != 0) {
      if (errorCallback) {
        var error;
        if (result.error == 1)
          error = new tizen.WebAPIError(tizen.WebAPIException.NOT_FOUND_ERR);
        else
          error = new tizen.WebAPIError(tizen.WebAPIException.UNKNOWN_ERR);
        errorCallback(error);
      }
      return;
    }

    var service = new BluetoothServiceHandler(uuid, name, result);
    adapter.service_handlers.push(service);

    if (serviceSuccessCallback) {
      serviceSuccessCallback(service);
    }
  });
};

var _deviceClassMask = {
  "MINOR": 0x3F,
  "MAJOR": 0x1F,
  "SERVICE": 0x7F9,
};

function BluetoothDevice(msg) {
  if (!msg) {
    _addConstProperty(this, "name", "");
    _addConstProperty(this, "address", "");
    _addConstProperty(this, "deviceClass", new BluetoothClass());
    _addConstProperty(this, "isBonded", false);
    _addConstProperty(this, "isTrusted", false);
    _addConstProperty(this, "isConnected", false);
    _addConstProperty(this, "uuids", []);
    return;
  }

  _addConstProperty(this, "name", msg.Name);
  _addConstProperty(this, "address", msg.Address);

  _addConstProperty(this, "deviceClass", new BluetoothClass());
  _addConstProperty(this.deviceClass, "minor", (msg.Class >> 2) & _deviceClassMask.MINOR);
  _addConstProperty(this.deviceClass, "major", (msg.Class >> 8) & _deviceClassMask.MAJOR);

  _addConstProperty(this, "isBonded", (msg.Paired == "true") ? true : false);
  _addConstProperty(this, "isTrusted", (msg.Trusted == "true") ? true : false);
  _addConstProperty(this, "isConnected", (msg.Connected == "true") ? true : false);
  // Parse UUIDs
  var uuids_array = [];
  if (msg.UUIDs) {
    uuids_array = msg.UUIDs.substring(msg.UUIDs.indexOf("[") + 1,
        msg.UUIDs.indexOf("]")).split(",");
    for (var i=0; i < uuids_array.length; i++) {
      uuids_array[i] = uuids_array[i].substring(2, uuids_array[i].length - 1);
    }
  }
  _addConstProperty(this, "uuids", uuids_array);

  var services = (msg.Class >> 13) & _deviceClassMask.SERVICE;
  var services_array = [];

  // 11 is the number of bits in _deviceClassMask.SERVICE
  for (var i = 0; i < 11; i++)
    if ((services & (1 << i)) !== 0)
      services_array.push(1 << i);

  _addConstProperty(this.deviceClass, "services", services_array);
};

BluetoothDevice.prototype.connectToServiceByUUID = function(uuid, socketSuccessCallback, errorCallback) {};

BluetoothDevice.prototype._clone = function() {
  var clone = new BluetoothDevice();
  _addConstProperty(clone, "name", this.name);
  _addConstProperty(clone, "address", this.address);
  _addConstProperty(clone, "deviceClass", this.deviceClass);
  _addConstProperty(clone, "isBonded", this.isBonded);
  _addConstProperty(clone, "isTrusted", this.isTrusted);
  _addConstProperty(clone, "isConnected", this.isConnected);

  var uuids_array = [];
  for (var i = 0; i < this.uuids.length; i++)
    uuids_array[i] = this.uuids[i];

  _addConstProperty(clone, "uuids", uuids_array);

  return clone;
};

BluetoothDevice.prototype._updateProperties = function(device) {
  if (device.hasOwnProperty("name"))
    _addConstProperty(this, "name", device.name);
  if (device.hasOwnProperty("address"))
    _addConstProperty(this, "address", device.address);
  if (device.hasOwnProperty("deviceClass"))
    _addConstProperty(this, "deviceClass", device.deviceClass);
  if (device.hasOwnProperty("isBonded"))
    _addConstProperty(this, "isBonded", device.isBonded);
  if (device.hasOwnProperty("isTrusted"))
    _addConstProperty(this, "isTrusted", device.isTrusted);
  if (device.hasOwnProperty("isConnected"))
    _addConstProperty(this, "isConnected", device.isConnected);

  if (device.hasOwnProperty("uuids")) {
    for (var i = 0; i < this.uuids.length; i++)
      this.uuids[i] = device.uuids[i];
  }
};

function BluetoothSocket(uuid, peer, msg) {
  _addConstProperty(this, "uuid", uuid);
  _addConstProperty(this, "peer", peer);
  _addConstProperty(this, "state", this.BluetoothSocketState.OPEN);
  this.onclose = null;
  this.onmessage = null;
  this.data = [];
  this.channel = 0;
  this.socket_fd = 0;

  if (msg) {
    this.channel = msg.channel;
    this.socket_fd = msg.socket_fd;
  }
};

BluetoothSocket.prototype.BluetoothSocketState = {};
var BluetoothSocketState = {
  "CLOSE": 1,
  "OPEN": 2,
};
for (var key in BluetoothSocketState) {
  Object.defineProperty(BluetoothSocket.prototype.BluetoothSocketState, key, {
    configurable: false,
    writable: false,
    value: BluetoothSocketState[key]
  });
};

BluetoothSocket.prototype.writeData = function(data) {/*return ulong*/};
BluetoothSocket.prototype.readData = function() {/*return byte[]*/};
BluetoothSocket.prototype.close = function() {/*return byte[]*/};

function BluetoothClass() {};
BluetoothClass.prototype.hasService = function(service) {
  for (var i = 0; i < this.services.length; i++)
    if (this.services[i] === service)
      return true;

  return false;
};

function BluetoothServiceHandler(name, uuid, msg) {
  _addConstProperty(this, "name", name);
  _addConstProperty(this, "uuid", uuid);
  _addConstProperty(this, "isConnected", false);

  if (msg) {
    this.server_fd = msg.server_fd;
    this.sdp_handle = msg.sdp_handle;
    this.channel = msg.channel;
  }
};
BluetoothServiceHandler.prototype.unregister = function(successCallback, errorCallback) {};

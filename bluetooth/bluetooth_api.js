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
};

Adapter.prototype.serviceNotAvailable = function(errorCallback) {
  if (adapter.isReady)
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
  if (on_discovery) {
    var index = this.indexOfDevice(this.found_devices, device.address);
    if (index == -1)
      this.found_devices.push(device);
    else
      this.found_devices[index] = device;
  }

  var i = this.indexOfDevice(this.known_devices, device.address);
  if (i == -1)
    this.known_devices.push(device);
  else
    this.known_devices[i] = device;
};

Adapter.prototype.updateDevice = function(device) {
  var index = this.indexOfDevice(this.known_devices, device.address);
  if (index != -1)
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

  adapter.addDevice(device, msg.found_on_discovery);

  // FIXME(jeez): we are not returning a deep copy so we can keep
  // the devices up-to-date. We have to find a better way to handle this.
  if (msg.found_on_discovery)
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
      defaultAdapter.name = msg.Name;
    if (msg.Address)
      defaultAdapter.address = msg.Address;
    if (msg.Powered) {
      defaultAdapter.powered =
          (msg.Powered == "true") ? true : false;
    }
    if (msg.Discoverable) {
      defaultAdapter.visible =
          (msg.Discoverable == "true") ? true : false;
    }
};

var defaultAdapter = new BluetoothAdapter();

exports.getDefaultAdapter = function() {
  var msg = {
    'cmd': 'GetDefaultAdapter'
  };
  var result = JSON.parse(extension.internal.sendSyncMessage(JSON.stringify(msg)));
  defaultAdapter.name = result.name;
  defaultAdapter.address = result.address;
  defaultAdapter.powered = result.powered;
  defaultAdapter.visible = result.visible;

  if (result.hasOwnProperty("address") && result.address != "")
    adapter.isReady = true;

  return defaultAdapter;
};

exports.deviceMajor = {
  "MISC": 0x00,
  "COMPUTER": 0x01,
  "PHONE": 0x02,
  "NETWORK": 0x03,
  "AUDIO_VIDEO": 0x04,
  "PERIPHERAL": 0x05,
  "IMAGING": 0x06,
  "WEARABLE": 0x07,
  "TOY": 0x08,
  "HEALTH": 0x09,
  "UNCATEGORIZED": 0x1F
};

exports.deviceMinor = {
  "COMPUTER_UNCATEGORIZED": 0x00,
  "COMPUTER_DESKTOP": 0x01,
  "COMPUTER_SERVER": 0x02,
  "COMPUTER_LAPTOP": 0x03,
  "COMPUTER_HANDHELD_PC_OR_PDA": 0x04,
  "COMPUTER_PALM_PC_OR_PDA": 0x05,
  "COMPUTER_WEARABLE": 0x06,
  "PHONE_UNCATEGORIZED": 0x00,
  "PHONE_CELLULAR": 0x01,
  "PHONE_CORDLESS": 0x02,
  "PHONE_SMARTPHONE": 0x03,
  "PHONE_MODEM_OR_GATEWAY": 0x04,
  "PHONE_ISDN": 0x05,
  "AV_UNRECOGNIZED": 0x00,
  "AV_WEARABLE_HEADSET": 0x01,
  "AV_HANDSFREE": 0x02,
  "AV_MICROPHONE": 0x04,
  "AV_LOUDSPEAKER": 0x05,
  "AV_HEADPHONES": 0x06,
  "AV_PORTABLE_AUDIO": 0x07,
  "AV_CAR_AUDIO": 0x08,
  "AV_SETTOP_BOX": 0x09,
  "AV_HIFI": 0x0a,
  "AV_VCR": 0x0b,
  "AV_VIDEO_CAMERA": 0x0c,
  "AV_CAMCORDER": 0x0d,
  "AV_MONITOR": 0x0e,
  "AV_DISPLAY_AND_LOUDSPEAKER": 0x0f,
  "AV_VIDEO_CONFERENCING": 0x10,
  "AV_GAMING_TOY": 0x12,
  "PERIPHERAL_UNCATEGORIZED": 0,
  "PERIPHERAL_KEYBOARD": 0x10,
  "PERIPHERAL_POINTING_DEVICE": 0x20,
  "PERIPHERAL_KEYBOARD_AND_POINTING_DEVICE": 0x30,
  "PERIPHERAL_JOYSTICK": 0x01,
  "PERIPHERAL_GAMEPAD": 0x02,
  "PERIPHERAL_REMOTE_CONTROL": 0x03,
  "PERIPHERAL_SENSING_DEVICE": 0x04,
  "PERIPHERAL_DEGITIZER_TABLET": 0x05,
  "PERIPHERAL_CARD_READER": 0x06,
  "PERIPHERAL_DIGITAL_PEN": 0x07,
  "PERIPHERAL_HANDHELD_SCANNER": 0x08,
  "PERIPHERAL_HANDHELD_INPUT_DEVICE": 0x09,
  "IMAGING_UNCATEGORIZED": 0x00,
  "IMAGING_DISPLAY": 0x04,
  "IMAGING_CAMERA": 0x08,
  "IMAGING_SCANNER": 0x10,
  "IMAGING_PRINTER": 0x20,
  "WEARABLE_WRITST_WATCH": 0x01,
  "WEARABLE_PAGER": 0x02,
  "WEARABLE_JACKET": 0x03,
  "WEARABLE_HELMET": 0x04,
  "WEARABLE_GLASSES": 0x05,
  "TOY_ROBOT": 0x01,
  "TOY_VEHICLE": 0x02,
  "TOY_DOLL": 0x03,
  "TOY_CONTROLLER": 0x04,
  "TOY_GAME": 0x05,
  "HEALTH_UNDEFINED": 0x00,
  "HEALTH_BLOOD_PRESSURE_MONITOR": 0x01,
  "HEALTH_THERMOMETER": 0x02,
  "HEALTH_WEIGHING_SCALE": 0x03,
  "HEALTH_GLUCOSE_METER": 0x04,
  "HEALTH_PULSE_OXIMETER": 0x05,
  "HEALTH_PULSE_RATE_MONITOR": 0x06,
  "HEALTH_DATA_DISPLAY": 0x07,
  "HEALTH_STEP_COUNTER": 0x08,
  "HEALTH_BODY_COMPOSITION_ANALYZER": 0x09,
  "HEALTH_PEAK_FLOW_MONITOR": 0x0a,
  "HEALTH_MEDICATION_MONITOR": 0x0b,
  "HEALTH_KNEE_PROSTHESIS": 0x0c,
  "HEALTH_ANKLE_PROSTHESIS": 0x0d
};

exports.deviceService = {
  "LIMITED_DISCOVERABILITY": 0x0001,
  "POSITIONING": 0x0008,
  "NETWORKING": 0x0010,
  "RENDERING": 0x0020,
  "CAPTURING": 0x0040,
  "OBJECT_TRANSFER": 0x0080,
  "AUDIO": 0x0100,
  "TELEPHONY": 0x0200,
  "INFORMATION": 0x0400
};

function BluetoothAdapter() {
  this.name = "";
  this.address = "00:00:00:00:00:00";
  this.powered = false;
  this.visible = false;
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
  if (adapter.serviceNotAvailable(errorCallback))
    return;

  if (typeof state !== 'boolean'
        || (successCallback && typeof successCallback !== 'function')
        || (errorCallback && typeof errorCallback !== 'function')) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  var msg = {
    'cmd': 'SetAdapterProperty',
    'property': 'Powered',
    'value': state
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

BluetoothAdapter.prototype.getDevice = function(address, deviceSuccessCallback, errorCallback) {};
BluetoothAdapter.prototype.createBonding = function(address, deviceSuccessCallback, errorCallback) {};
BluetoothAdapter.prototype.destroyBonding = function(address, deviceSuccessCallback, errorCallback) {};
BluetoothAdapter.prototype.registerRFCOMMServiceByUUID = function(uuid, name, serviceSuccessCallback, errorCallback) {};

var _deviceClassMask = {
  "MINOR": 0x3F,
  "MAJOR": 0x1F,
};

function BluetoothDevice(msg) {
  if (!msg) {
    this.name = "";
    this.address = "";
    this.deviceClass = new BluetoothClass();
    this.isBonded = false;
    this.isTrusted = false;
    this.isConnected = false;
    this.uuids = [];
    return;
  }

  this.name = msg.Name;
  this.address = msg.Address;

  this.deviceClass = new BluetoothClass();
  this.deviceClass.minor = (msg.Class >> 2) & _deviceClassMask.MINOR;
  this.deviceClass.major = (msg.Class >> 8) & _deviceClassMask.MAJOR;

  this.isBonded = (msg.Paired == "true") ? true : false;
  this.isTrusted = (msg.Trusted == "true") ? true : false;
  this.isConnected = (msg.Connected == "true") ? true : false;
  // Parse UUIDs
  var uuids_array = [];
  if (msg.UUIDs) {
    uuids_array = msg.UUIDs.substring(msg.UUIDs.indexOf("[") + 1,
        msg.UUIDs.indexOf("]")).split(",");
    for (var i=0; i < uuids_array.length; i++) {
      uuids_array[i] = uuids_array[i].substring(2, uuids_array[i].length - 1);
    }
  }
  this.uuids = uuids_array;
};

BluetoothDevice.prototype.connectToServiceByUUID = function(uuid, socketSuccessCallback, errorCallback) {};

BluetoothDevice.prototype._clone = function() {
  var clone = new BluetoothDevice();
  clone.name = this.name;
  clone.address = this.address;
  clone.deviceClass = this.deviceClass;
  clone.isBonded = this.isBonded;
  clone.isTrusted = this.isTrusted;
  clone.isConnected = this.isConnected;
  clone.uuids = [];
  for (var i = 0; i < this.uuids.length; i++)
    clone.uuids[i] = this.uuids[i];

  return clone;
};

BluetoothDevice.prototype._updateProperties = function(device) {
  if (device.hasOwnProperty("name"))
    this.name = device.name;
  if (device.hasOwnProperty("address"))
    this.address = device.address;
  if (device.hasOwnProperty("deviceClass"))
    this.deviceClass = device.deviceClass;
  if (device.hasOwnProperty("isBonded"))
    this.isBonded = device.isBonded;
  if (device.hasOwnProperty("isTrusted"))
    this.isTrusted = device.isTrusted;
  if (device.hasOwnProperty("isConnected"))
    this.isConnected = device.isConnected;

  if (device.hasOwnProperty("uuids")) {
    for (var i = 0; i < this.uuids.length; i++)
      this.uuids[i] = device.uuids[i];
  }
};

function BluetoothSocket() {};
BluetoothSocket.prototype.writeData = function(data) {/*return ulong*/};
BluetoothSocket.prototype.readData = function() {/*return byte[]*/};
BluetoothSocket.prototype.close = function() {/*return byte[]*/};

function BluetoothClass() {};
BluetoothClass.prototype.services = [];
BluetoothClass.prototype.hasService = function(service) {/*return bool*/};

function BluetoothServiceHandler() {};
BluetoothServiceHandler.prototype.unregister = function(successCallback, errorCallback) {};

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
  if (msg.cmd == 'BondedDevice')
    handleBondedDevice(msg);
  else if (msg.cmd == 'DeviceFound')
    handleDeviceFound(msg);
  else if (msg.cmd == 'DiscoveryFinished')
    handleDiscoveryFinished();
  else if (msg.cmd == 'DeviceRemoved')
    handleDeviceRemoved(msg.Address);
  else if (msg.cmd == 'DeviceUpdated')
    handleDeviceUpdated(msg);
  else if (msg.cmd == 'AdapterUpdated')
    handleAdapterUpdated(msg);
  else if (msg.cmd == 'RFCOMMSocketAccept')
    handleRFCOMMSocketAccept(msg);
  else if (msg.cmd == 'SocketHasData')
    handleSocketHasData(msg);
  else if (msg.cmd == 'SocketClosed')
    handleSocketClosed(msg);

  if (msg.reply_id) { // Then we are dealing with postMessage return.
    var reply_id = msg.reply_id;
    var callback = _callbacks[reply_id];
    if (callback) {
      delete msg.reply_id;
      delete _callbacks[reply_id];
      callback(msg);
    } else {
      // do not print error log when the postmessage was not initiated by JS
      if (reply_id != '')
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
  this.change_listener = {};
  this.health_apps = {};
  this.health_channel_listener = {};
}

function validateAddress(address) {
  if (typeof address !== 'string')
    return false;

  var regExp = /([\dA-F][\dA-F]:){5}[\dA-F][\dA-F]/i;

  if (!address.match(regExp))
    return false;

  return true;
}

Adapter.prototype.checkServiceAvailability = function(errorCallback) {
  if (adapter.isReady && defaultAdapter.powered)
    return false;

  if (errorCallback) {
    var error = new tizen.WebAPIError(tizen.WebAPIException.SERVICE_NOT_AVAILABLE_ERR);
    errorCallback(error);
  }

  return true;
};

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

var handleBondedDevice = function(msg) {
  var device = new BluetoothDevice(msg);
  adapter.addDevice(device, false);
};

var handleDeviceFound = function(msg) {
  var device = new BluetoothDevice(msg);
  var is_new = adapter.addDevice(device, msg.found_on_discovery);

  // FIXME(jeez): we are not returning a deep copy so we can keep
  // the devices up-to-date. We have to find a better way to handle this.
  if (is_new && msg.found_on_discovery && adapter.discovery_callbacks.ondevicefound)
    adapter.discovery_callbacks.ondevicefound(device);
};

var handleDiscoveryFinished = function() {
  // FIXME(jeez): we are not returning a deep copy so we can keep
  // the devices up-to-date. We have to find a better way to handle this.
  if (typeof adapter.discovery_callbacks.onfinished === 'function')
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

  if (adapter.discovery_callbacks.ondevicedisappeared)
    adapter.discovery_callbacks.ondevicedisappeared(address);
};

var handleDeviceUpdated = function(msg) {
  var device = new BluetoothDevice(msg);
  adapter.updateDevice(device);
};

var handleAdapterUpdated = function(msg) {
  var listener = adapter.change_listener;

  if (msg.Name) {
    _addConstProperty(defaultAdapter, 'name', msg.Name);
    if (listener && listener.onnamechanged) {
      adapter.change_listener.onnamechanged(msg.Name);
    }
  }

  if (msg.Address)
    _addConstProperty(defaultAdapter, 'address', msg.Address);

  if (msg.Powered) {
    var powered = (msg.Powered === 'true');
    _addConstProperty(defaultAdapter, 'powered', powered);
    if (listener && listener.onstatechanged) {
      adapter.change_listener.onstatechanged(powered);
    }
  }

  if (msg.Discoverable) {
    var visibility = (msg.Discoverable === 'true');

    if (defaultAdapter.visible !== visibility && listener && listener.onvisibilitychanged) {
      adapter.change_listener.onvisibilitychanged(visibility);
    }
    _addConstProperty(defaultAdapter, 'visible', visibility);
  }

  defaultAdapter.isReady = true;
};

var handleRFCOMMSocketAccept = function(msg) {
  for (var i in adapter.service_handlers) {
    var server = adapter.service_handlers[i];
    // FIXME(clecou) BlueZ4 backend compares rfcomm channel number but this parameter
    // is not available in Tizen C API so we check socket fd.
    // A better approach would be to adapt backends instances to have a single JSON protocol.
    if (server.channel === msg.channel || server.server_fd === msg.socket_fd) {
      var j = adapter.indexOfDevice(adapter.known_devices, msg.peer);
      var peer = adapter.known_devices[j];

      var socket = new BluetoothSocket(server.uuid, peer, msg);

      adapter.sockets.push(socket);

      _addConstProperty(server, 'isConnected', true);

      if (server.onconnect && typeof server.onconnect === 'function')
        server.onconnect(socket);
      return;
    }
  }
};

var handleSocketHasData = function(msg) {
  for (var i in adapter.sockets) {
    var socket = adapter.sockets[i];
    if (socket.socket_fd === msg.socket_fd) {
      socket.data = msg.data;

      if (socket.onmessage && typeof socket.onmessage === 'function')
        socket.onmessage();

      socket.data = [];
      return;
    }
  }
};

var handleSocketClosed = function(msg) {
  for (var i in adapter.sockets) {
    var socket = adapter.sockets[i];
    if (socket.socket_fd === msg.socket_fd) {
      if (socket.onclose && typeof socket.onmessage === 'function')
        socket.onclose();

      return;
    }
  }
};

function _addConstProperty(obj, propertyKey, propertyValue) {
  Object.defineProperty(obj, propertyKey, {
    configurable: true,
    writable: false,
    value: propertyValue
  });
}

exports.deviceMajor = {};
var deviceMajor = {
  'MISC': { value: 0x00, configurable: false, writable: false },
  'COMPUTER': { value: 0x01, configurable: false, writable: false },
  'PHONE': { value: 0x02, configurable: false, writable: false },
  'NETWORK': { value: 0x03, configurable: false, writable: false },
  'AUDIO_VIDEO': { value: 0x04, configurable: false, writable: false },
  'PERIPHERAL': { value: 0x05, configurable: false, writable: false },
  'IMAGING': { value: 0x06, configurable: false, writable: false },
  'WEARABLE': { value: 0x07, configurable: false, writable: false },
  'TOY': { value: 0x08, configurable: false, writable: false },
  'HEALTH': { value: 0x09, configurable: false, writable: false },
  'UNCATEGORIZED': { value: 0x1F, configurable: false, writable: false }
};
Object.defineProperties(exports.deviceMajor, deviceMajor);
_addConstProperty(exports, 'deviceMajor', exports.deviceMajor);

exports.deviceMinor = {};
var deviceMinor = {
  'COMPUTER_UNCATEGORIZED': { value: 0x00, configurable: false, writable: false },
  'COMPUTER_DESKTOP': { value: 0x01, configurable: false, writable: false },
  'COMPUTER_SERVER': { value: 0x02, configurable: false, writable: false },
  'COMPUTER_LAPTOP': { value: 0x03, configurable: false, writable: false },
  'COMPUTER_HANDHELD_PC_OR_PDA': { value: 0x04, configurable: false, writable: false },
  'COMPUTER_PALM_PC_OR_PDA': { value: 0x05, configurable: false, writable: false },
  'COMPUTER_WEARABLE': { value: 0x06, configurable: false, writable: false },
  'PHONE_UNCATEGORIZED': { value: 0x00, configurable: false, writable: false },
  'PHONE_CELLULAR': { value: 0x01, configurable: false, writable: false },
  'PHONE_CORDLESS': { value: 0x02, configurable: false, writable: false },
  'PHONE_SMARTPHONE': { value: 0x03, configurable: false, writable: false },
  'PHONE_MODEM_OR_GATEWAY': { value: 0x04, configurable: false, writable: false },
  'PHONE_ISDN': { value: 0x05, configurable: false, writable: false },
  'AV_UNRECOGNIZED': { value: 0x00, configurable: false, writable: false },
  'AV_WEARABLE_HEADSET': { value: 0x01, configurable: false, writable: false },
  'AV_HANDSFREE': { value: 0x02, configurable: false, writable: false },
  'AV_MICROPHONE': { value: 0x04, configurable: false, writable: false },
  'AV_LOUDSPEAKER': { value: 0x05, configurable: false, writable: false },
  'AV_HEADPHONES': { value: 0x06, configurable: false, writable: false },
  'AV_PORTABLE_AUDIO': { value: 0x07, configurable: false, writable: false },
  'AV_CAR_AUDIO': { value: 0x08, configurable: false, writable: false },
  'AV_SETTOP_BOX': { value: 0x09, configurable: false, writable: false },
  'AV_HIFI': { value: 0x0a, configurable: false, writable: false },
  'AV_VCR': { value: 0x0b, configurable: false, writable: false },
  'AV_VIDEO_CAMERA': { value: 0x0c, configurable: false, writable: false },
  'AV_CAMCORDER': { value: 0x0d, configurable: false, writable: false },
  'AV_MONITOR': { value: 0x0e, configurable: false, writable: false },
  'AV_DISPLAY_AND_LOUDSPEAKER': { value: 0x0f, configurable: false, writable: false },
  'AV_VIDEO_CONFERENCING': { value: 0x10, configurable: false, writable: false },
  'AV_GAMING_TOY': { value: 0x12, configurable: false, writable: false },
  'PERIPHERAL_UNCATEGORIZED': { value: 0, configurable: false, writable: false },
  'PERIPHERAL_KEYBOARD': { value: 0x10, configurable: false, writable: false },
  'PERIPHERAL_POINTING_DEVICE': { value: 0x20, configurable: false, writable: false },
  'PERIPHERAL_KEYBOARD_AND_POINTING_DEVICE': { value: 0x30, configurable: false, writable: false },
  'PERIPHERAL_JOYSTICK': { value: 0x01, configurable: false, writable: false },
  'PERIPHERAL_GAMEPAD': { value: 0x02, configurable: false, writable: false },
  'PERIPHERAL_REMOTE_CONTROL': { value: 0x03, configurable: false, writable: false },
  'PERIPHERAL_SENSING_DEVICE': { value: 0x04, configurable: false, writable: false },
  'PERIPHERAL_DEGITIZER_TABLET': { value: 0x05, configurable: false, writable: false },
  'PERIPHERAL_CARD_READER': { value: 0x06, configurable: false, writable: false },
  'PERIPHERAL_DIGITAL_PEN': { value: 0x07, configurable: false, writable: false },
  'PERIPHERAL_HANDHELD_SCANNER': { value: 0x08, configurable: false, writable: false },
  'PERIPHERAL_HANDHELD_INPUT_DEVICE': { value: 0x09, configurable: false, writable: false },
  'IMAGING_UNCATEGORIZED': { value: 0x00, configurable: false, writable: false },
  'IMAGING_DISPLAY': { value: 0x04, configurable: false, writable: false },
  'IMAGING_CAMERA': { value: 0x08, configurable: false, writable: false },
  'IMAGING_SCANNER': { value: 0x10, configurable: false, writable: false },
  'IMAGING_PRINTER': { value: 0x20, configurable: false, writable: false },
  'WEARABLE_WRITST_WATCH': { value: 0x01, configurable: false, writable: false },
  'WEARABLE_PAGER': { value: 0x02, configurable: false, writable: false },
  'WEARABLE_JACKET': { value: 0x03, configurable: false, writable: false },
  'WEARABLE_HELMET': { value: 0x04, configurable: false, writable: false },
  'WEARABLE_GLASSES': { value: 0x05, configurable: false, writable: false },
  'TOY_ROBOT': { value: 0x01, configurable: false, writable: false },
  'TOY_VEHICLE': { value: 0x02, configurable: false, writable: false },
  'TOY_DOLL': { value: 0x03, configurable: false, writable: false },
  'TOY_CONTROLLER': { value: 0x04, configurable: false, writable: false },
  'TOY_GAME': { value: 0x05, configurable: false, writable: false },
  'HEALTH_UNDEFINED': { value: 0x00, configurable: false, writable: false },
  'HEALTH_BLOOD_PRESSURE_MONITOR': { value: 0x01, configurable: false, writable: false },
  'HEALTH_THERMOMETER': { value: 0x02, configurable: false, writable: false },
  'HEALTH_WEIGHING_SCALE': { value: 0x03, configurable: false, writable: false },
  'HEALTH_GLUCOSE_METER': { value: 0x04, configurable: false, writable: false },
  'HEALTH_PULSE_OXIMETER': { value: 0x05, configurable: false, writable: false },
  'HEALTH_PULSE_RATE_MONITOR': { value: 0x06, configurable: false, writable: false },
  'HEALTH_DATA_DISPLAY': { value: 0x07, configurable: false, writable: false },
  'HEALTH_STEP_COUNTER': { value: 0x08, configurable: false, writable: false },
  'HEALTH_BODY_COMPOSITION_ANALYZER': { value: 0x09, configurable: false, writable: false },
  'HEALTH_PEAK_FLOW_MONITOR': { value: 0x0a, configurable: false, writable: false },
  'HEALTH_MEDICATION_MONITOR': { value: 0x0b, configurable: false, writable: false },
  'HEALTH_KNEE_PROSTHESIS': { value: 0x0c, configurable: false, writable: false },
  'HEALTH_ANKLE_PROSTHESIS': { value: 0x0d, configurable: false, writable: false }
};
Object.defineProperties(exports.deviceMinor, deviceMinor);
_addConstProperty(exports, 'deviceMinor', exports.deviceMinor);

exports.deviceService = {};
var deviceService = {
  'LIMITED_DISCOVERABILITY': { value: 0x0001, configurable: false, writable: false },
  'POSITIONING': { value: 0x0008, configurable: false, writable: false },
  'NETWORKING': { value: 0x0010, configurable: false, writable: false },
  'RENDERING': { value: 0x0020, configurable: false, writable: false },
  'CAPTURING': { value: 0x0040, configurable: false, writable: false },
  'OBJECT_TRANSFER': { value: 0x0080, configurable: false, writable: false },
  'AUDIO': { value: 0x0100, configurable: false, writable: false },
  'TELEPHONY': { value: 0x0200, configurable: false, writable: false },
  'INFORMATION': { value: 0x0400, configurable: false, writable: false }
};
Object.defineProperties(exports.deviceService, deviceService);
_addConstProperty(exports, 'deviceService', exports.deviceService);

var defaultAdapter = new BluetoothAdapter();

exports.getDefaultAdapter = function() {
  var msg = {
    'cmd': 'GetDefaultAdapter'
  };
  var result = JSON.parse(extension.internal.sendSyncMessage(JSON.stringify(msg)));

  if (!result.error) {
    _addConstProperty(defaultAdapter, 'name', result.name);
    _addConstProperty(defaultAdapter, 'address', result.address);
    _addConstProperty(defaultAdapter, 'powered', result.powered);
    _addConstProperty(defaultAdapter, 'visible', result.visible);

    if (result.hasOwnProperty('address') && result.address != '')
      adapter.isReady = true;
  } else {
    adapter.isReady = false;
    throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);
  }

  return defaultAdapter;
};

function BluetoothAdapter() {
  _addConstProperty(this, 'name', '');
  _addConstProperty(this, 'address', '00:00:00:00:00:00');
  _addConstProperty(this, 'powered', false);
  _addConstProperty(this, 'visible', false);
}

BluetoothAdapter.prototype.setName = function(name, successCallback, errorCallback) {
  if (!xwalk.utils.validateArguments('s?ff', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  if (adapter.checkServiceAvailability(errorCallback))
    return;

  var msg = {
    'cmd': 'SetAdapterProperty',
    'property': 'Name',
    'value': name
  };

  postMessage(msg, function(result) {
    if (result.error != 0) {
      if (errorCallback) {
        var error = new tizen.WebAPIError(tizen.WebAPIException.UNKNOWN_ERR);
        if (result.error == 1)
          error = new tizen.WebAPIError(tizen.WebAPIException.INVALID_VALUES_ERR);
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
  if (!xwalk.utils.validateArguments('b?ff', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  if ((state === defaultAdapter.powered) && successCallback) {
    successCallback();
    return;
  }

  var msg = {
    'cmd': 'SetAdapterProperty',
    'property': 'Powered',
    'value': state
  };

  postMessage(msg, function(result) {
    if (result.error) {
      if (errorCallback) {
        var error = new tizen.WebAPIError(tizen.WebAPIException.INVALID_VALUES_ERR);
        errorCallback(error);
        return;
      }

      throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
    }

    if (successCallback)
      successCallback();
  });
};

BluetoothAdapter.prototype.setVisible = function(mode, successCallback, errorCallback, timeout) {
  if (!xwalk.utils.validateArguments('b?ffn', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  if (adapter.checkServiceAvailability(errorCallback))
    return;

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
        var error = new tizen.WebAPIError(tizen.WebAPIException.UNKNOWN_ERR);
        if (result.error == 1)
          error = new tizen.WebAPIError(tizen.WebAPIException.INVALID_VALUES_ERR);
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
  if (!xwalk.utils.validateArguments('o?f', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  if (!xwalk.utils.validateObject(discoverySuccessCallback, 'ffff',
      ['onstarted', 'ondevicefound', 'ondevicedisappeared', 'onfinished'])) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  if (adapter.checkServiceAvailability(errorCallback))
    return;

  var msg = {
    'cmd': 'DiscoverDevices'
  };
  postMessage(msg, function(result) {
    if (result.error) {
      if (errorCallback) {
        var error = new tizen.WebAPIError(tizen.WebAPIException.UNKNOWN_ERR);
        errorCallback(error);
      }
      return;
    }

    adapter.discovery_callbacks = discoverySuccessCallback;

    if (discoverySuccessCallback && discoverySuccessCallback.onstarted)
      discoverySuccessCallback.onstarted();
  });
};

BluetoothAdapter.prototype.stopDiscovery = function(successCallback, errorCallback) {
  if (!xwalk.utils.validateArguments('?ff', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  if (adapter.checkServiceAvailability(errorCallback))
    return;

  var msg = {
    'cmd': 'StopDiscovery'
  };
  postMessage(msg, function(result) {
    if (result.error) {
      if (errorCallback) {
        var error = new tizen.WebAPIError(tizen.WebAPIException.UNKNOWN_ERR);
        errorCallback(error);
      }
      return;
    }

    if (successCallback)
      successCallback();

    handleDiscoveryFinished();
  });
};

BluetoothAdapter.prototype.getKnownDevices = function(deviceArraySuccessCallback, errorCallback) {
  if (!xwalk.utils.validateArguments('f?f', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  if (adapter.checkServiceAvailability(errorCallback))
    return;

  // FIXME(jeez): we are not returning a deep copy so we can keep
  // the devices up-to-date. We have to find a better way to handle this.
  deviceArraySuccessCallback(adapter.known_devices);
};

BluetoothAdapter.prototype.getDevice = function(address, deviceSuccessCallback, errorCallback) {
  if (!xwalk.utils.validateArguments('sf?f', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  if (!validateAddress(address)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  if (adapter.checkServiceAvailability(errorCallback))
    return;

  var index = adapter.indexOfDevice(adapter.known_devices, address);
  if (index == -1) {
    var error = new tizen.WebAPIError(tizen.WebAPIException.NOT_FOUND_ERR);
    errorCallback(error);
    return;
  }

  deviceSuccessCallback(adapter.known_devices[index]);
};

BluetoothAdapter.prototype.createBonding = function(address, successCallback, errorCallback) {
  if (!xwalk.utils.validateArguments('sf?f', arguments)) {
    throw new tizen.WebAPIError(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  if (!validateAddress(address)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  if (adapter.checkServiceAvailability(errorCallback))
    return;

  var msg = {
    'cmd': 'CreateBonding',
    'address': address
  };

  postMessage(msg, function(result) {
    var cb_device;

    if (result.error) {
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

      // FIXME(clecou) Update known device state here when using C API Tizen backend
      // BlueZ backends update the device state automatically when catching dbus signals.
      // A better approach would be to adapt backends instances to have a single JSON protocol.
      if (result.capi)
        _addConstProperty(adapter.known_devices[i], 'isBonded', true);

      successCallback(cb_device);
    }
  });
};

BluetoothAdapter.prototype.destroyBonding = function(address, successCallback, errorCallback) {
  if (!xwalk.utils.validateArguments('s?ff', arguments)) {
    throw new tizen.WebAPIError(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  if (!validateAddress(address)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  if (adapter.checkServiceAvailability(errorCallback))
    return;

  var msg = {
    'cmd': 'DestroyBonding',
    'address': address
  };

  postMessage(msg, function(result) {
    var cb_device;

    if (result.error != 0) {
      if (errorCallback) {
        var error_type = (result.error == 1) ?
            tizen.WebAPIException.NOT_FOUND_ERR : tizen.WebAPIException.UNKNOWN_ERR;
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

      // FIXME(clecou) Update known device state here when using C API Tizen backend
      // BlueZ backends update the device state automatically when catching dbus signals
      // A better approach would be to adapt backends instances to have a single JSON protocol.
      if (result.capi)
        _addConstProperty(adapter.known_devices[i], 'isBonded', false);

      successCallback(cb_device);
    }
  });
};

BluetoothAdapter.prototype.registerRFCOMMServiceByUUID =
    function(uuid, name, serviceSuccessCallback, errorCallback) {
  if (!xwalk.utils.validateArguments('ssf?f', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  if (adapter.checkServiceAvailability(errorCallback))
    return;

  var msg = {
    'cmd': 'RFCOMMListen',
    'uuid': uuid,
    'name': name
  };

  postMessage(msg, function(result) {
    if (result.error != 0) {
      if (errorCallback) {
        var error = new tizen.WebAPIError(tizen.WebAPIException.UNKNOWN_ERR);
        if (result.error == 1)
          error = new tizen.WebAPIError(tizen.WebAPIException.INVALID_VALUES_ERR);
        errorCallback(error);
      }
      return;
    }

    var service = new BluetoothServiceHandler(uuid.toUpperCase(), name, result);
    adapter.service_handlers.push(service);

    if (serviceSuccessCallback) {
      serviceSuccessCallback(service);
    }
  });
};

BluetoothAdapter.prototype.setChangeListener = function(listener) {
  if (!xwalk.utils.validateArguments('o', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  if (!xwalk.utils.validateObject(listener, 'fff',
      ['onstatechanged', 'onnamechanged', 'onvisibilitychanged'])) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  adapter.change_listener = listener;
};

BluetoothAdapter.prototype.unsetChangeListener = function() {
  adapter.change_listener = {};
};

function BluetoothProfileHandler(profileType) {
  _addConstProperty(this, 'profileType', profileType);
}

function BluetoothHealthProfileHandler() {
  BluetoothProfileHandler.call(this, 'HEALTH');
}
BluetoothHealthProfileHandler.prototype = Object.create(BluetoothProfileHandler.prototype);
BluetoothHealthProfileHandler.prototype.constructor = BluetoothHealthProfileHandler;

BluetoothAdapter.prototype.getBluetoothProfileHandler = function(profile_type) {
  if (!xwalk.utils.validateArguments('s', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  return (profile_type === 'HEALTH') ? new BluetoothHealthProfileHandler() :
      new BluetoothProfileHandler(profile_type);
};

var _deviceClassMask = {
  'MINOR': 0x3F,
  'MAJOR': 0x1F,
  'SERVICE': 0x7F9
};

function BluetoothDevice(msg) {
  if (!msg) {
    _addConstProperty(this, 'name', '');
    _addConstProperty(this, 'address', '');
    _addConstProperty(this, 'deviceClass', new BluetoothClass());
    _addConstProperty(this, 'isBonded', false);
    _addConstProperty(this, 'isTrusted', false);
    _addConstProperty(this, 'isConnected', false);
    _addConstProperty(this, 'uuids', []);
    return;
  }

  _addConstProperty(this, 'name', msg.Alias);
  _addConstProperty(this, 'address', msg.Address);

  _addConstProperty(this, 'deviceClass', new BluetoothClass());
  _addConstProperty(this.deviceClass, 'minor', (msg.ClassMinor >> 2) & _deviceClassMask.MINOR);
  _addConstProperty(this.deviceClass, 'major', msg.ClassMajor & _deviceClassMask.MAJOR);

  _addConstProperty(this, 'isBonded', (msg.Paired == 'true'));
  _addConstProperty(this, 'isTrusted', (msg.Trusted == 'true'));
  _addConstProperty(this, 'isConnected', (msg.Connected == 'true'));

  if (msg.UUIDs) {
    var uuids_array = [];
    if (typeof msg.UUIDs === 'string') {
      // FIXME(clecou) BlueZ backend sends a string to convert it into an array
      // A better approach would be to adapt backends instances to have a single JSON protocol.
      uuids_array = msg.UUIDs.substring(msg.UUIDs.indexOf('[') + 1,
          msg.UUIDs.indexOf(']')).split(',');
      for (var i = 0; i < uuids_array.length; i++) {
        uuids_array[i] = uuids_array[i].substring(2, uuids_array[i].length - 1);
      }

    } else {
      // Tizen C API backend directly sends an array
      uuids_array = msg.UUIDs;
      for (var i = 0; i < msg.UUIDs.length; i++)
        _addConstProperty(uuids_array, i.toString(), msg.UUIDs[i].toUpperCase());
    }
    _addConstProperty(this, 'uuids', uuids_array);
  }

  var services = (msg.ClassService >> 13) & _deviceClassMask.SERVICE;
  var services_array = [];
  var index = 0;

  var SERVICE_CLASS_BITS_NUMBER = 11;

  for (var i = 0; i < SERVICE_CLASS_BITS_NUMBER; i++) {
    if ((services & (1 << i)) !== 0) {
      _addConstProperty(services_array, index.toString(), (1 << i));
      index++;
    }
  }

  _addConstProperty(this.deviceClass, 'services', services_array);
}

BluetoothDevice.prototype.connectToServiceByUUID =
    function(uuid, socketSuccessCallback, errorCallback) {

  if (!xwalk.utils.validateArguments('sf?f', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  if (adapter.checkServiceAvailability(errorCallback))
    return;

  var uuid_found = false;
  for (var i = 0; i < this.uuids.length; i++) {
    if (this.uuids[i] == uuid.toUpperCase()) {
      uuid_found = true;
      break;
    }
  }
  if (uuid_found == false) {
    var error = new tizen.WebAPIError(tizen.WebAPIException.NOT_FOUND_ERR);
    errorCallback(error);
  }

  var msg = {
    'cmd': 'ConnectToService',
    'uuid': uuid,
    'address' : this.address
  };

  postMessage(msg, function(result) {
    if (result.error != 0) {
      if (errorCallback) {
        var error = new tizen.WebAPIError(tizen.WebAPIException.UNKNOWN_ERR);
        if (result.error == 1)
          error = new tizen.WebAPIError(tizen.WebAPIException.INVALID_VALUES_ERR);
        errorCallback(error);
      }
      throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
      return;
    }

    var i = adapter.indexOfDevice(adapter.known_devices, result.peer);
    var socket = new BluetoothSocket(result.uuid, adapter.known_devices[i], result);
    adapter.sockets.push(socket);
    socketSuccessCallback(socket);
  });
};

BluetoothDevice.prototype._clone = function() {
  var clone = new BluetoothDevice();
  _addConstProperty(clone, 'name', this.name);
  _addConstProperty(clone, 'address', this.address);
  _addConstProperty(clone, 'deviceClass', this.deviceClass);
  _addConstProperty(clone, 'isBonded', this.isBonded);
  _addConstProperty(clone, 'isTrusted', this.isTrusted);
  _addConstProperty(clone, 'isConnected', this.isConnected);

  var uuids_array = [];
  for (var i = 0; i < this.uuids.length; i++)
    uuids_array[i] = this.uuids[i];

  _addConstProperty(clone, 'uuids', uuids_array);

  return clone;
};

BluetoothDevice.prototype._updateProperties = function(device) {
  if (device.hasOwnProperty('name'))
    _addConstProperty(this, 'name', device.name);
  if (device.hasOwnProperty('address'))
    _addConstProperty(this, 'address', device.address);
  if (device.hasOwnProperty('deviceClass'))
    _addConstProperty(this, 'deviceClass', device.deviceClass);
  if (device.hasOwnProperty('isBonded'))
    _addConstProperty(this, 'isBonded', device.isBonded);
  if (device.hasOwnProperty('isTrusted'))
    _addConstProperty(this, 'isTrusted', device.isTrusted);
  if (device.hasOwnProperty('isConnected'))
    _addConstProperty(this, 'isConnected', device.isConnected);

  if (device.hasOwnProperty('uuids')) {
    for (var i = 0; i < this.uuids.length; i++)
      this.uuids[i] = device.uuids[i];
  }
};

function BluetoothSocket(uuid, peer, msg) {
  _addConstProperty(this, 'uuid', uuid);
  _addConstProperty(this, 'peer', peer);
  _addConstProperty(this, 'state', 'OPEN');
  this.onclose = null;
  this.onmessage = null;
  this.data = [];
  this.channel = 0;
  this.socket_fd = 0;

  if (msg) {
    this.channel = msg.channel;
    this.socket_fd = msg.socket_fd;
  }
}

BluetoothSocket.prototype.writeData = function(data) {
  // make sure that socket is connected and opened.
  if (this.state == 'CLOSED') {
    throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);
    return;
  }

  var msg = {
    'cmd': 'SocketWriteData',
    'data': data,
    'socket_fd': this.socket_fd
  };
  var result = JSON.parse(extension.internal.sendSyncMessage(JSON.stringify(msg)));

  return result.size;
};

BluetoothSocket.prototype.readData = function() {
  return this.data;
};

BluetoothSocket.prototype.close = function() {
  var msg = {
    'cmd': 'CloseSocket',
    'socket_fd': this.socket_fd
  };

  postMessage(msg, function(result) {
    if (result.error) {
      console.log('Can\'t close socket (' + this.socket_fd + ').');
      throw new tizen.WebAPIException(tizen.WebAPIException.UNKNOWN_ERR);
      return;
    }

    // FIXME(clecou) Update socket object state only when using Tizen C API backend.
    // BlueZ4 backend independently updates socket state based on a dbus callback mechanism.
    // A better approach would be to adapt backends instances to have a single JSON protocol.
    if (result.capi) {
      for (var i in adapter.sockets) {
        var socket = adapter.sockets[i];
        if (socket.socket_fd === msg.socket_fd) {
          if (socket.onclose && typeof socket.onmessage === 'function') {
            _addConstProperty(adapter.sockets[i], 'state', 'CLOSED');
            socket.onclose();
          }
        }
      }
    }
  });
};

function BluetoothClass() {}
BluetoothClass.prototype.hasService = function(service) {
  for (var i = 0; i < this.services.length; i++)
    if (this.services[i] === service)
      return true;

    return false;
};

function BluetoothServiceHandler(name, uuid, msg) {
  _addConstProperty(this, 'name', name);
  _addConstProperty(this, 'uuid', uuid);
  _addConstProperty(this, 'isConnected', false);
  this.onconnect = null;

  if (msg) {
    this.server_fd = msg.server_fd;
    this.sdp_handle = msg.sdp_handle;
    this.channel = msg.channel;
  }
}

BluetoothServiceHandler.prototype.unregister = function(successCallback, errorCallback) {
  if (!xwalk.utils.validateArguments('?ff', arguments)) {
    throw new tizen.WebAPIError(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  var msg = {
    'cmd': 'UnregisterServer',
    'server_fd': this.server_fd,
    'sdp_handle': this.sdp_handle
  };

  postMessage(msg, function(result) {
    if (result.error) {
      if (errorCallback) {
        var error = new tizen.WebAPIError(tizen.WebAPIException.UNKNOWN_ERR);
        errorCallback(error);
      }

      throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
      return;
    }

    for (var i in adapter.service_handlers) {
      var service = adapter.service_handlers[i];
      if (service.server_fd == result.socket_fd)
        adapter.service_handlers.splice(i, 1);
    }

    if (successCallback)
      successCallback();
  });
};

function BluetoothHealthApplication(data_type, app_name, msg) {
  _addConstProperty(this, 'dataType', data_type);
  _addConstProperty(this, 'name', app_name);
  this.onconnect = null;

  if (msg)
    this.app_id = msg.app_id;
}

BluetoothHealthApplication.prototype.unregister =
    function(successCallback, errorCallback) {
  if (!xwalk.utils.validateArguments('?ff', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  if (adapter.checkServiceAvailability(errorCallback))
    return;

  var msg = {
    'cmd': 'UnregisterSinkApp',
    'app_id': this.app_id
  };

  var app = this;

  postMessage(msg, function(result) {
    if (result.error) {
      if (errorCallback) {
        var error = new tizen.WebAPIError(tizen.WebAPIException.UNKNOWN_ERR);
        errorCallback(error);
      }
    }
    if (app.app_id)
      delete adapter.health_apps[app.app_id];

    if (successCallback)
      successCallback();
  });
};

BluetoothHealthProfileHandler.prototype.registerSinkApplication =
    function(dataType, name, successCallback, errorCallback) {
  if (!xwalk.utils.validateArguments('nsf?f', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  if (adapter.checkServiceAvailability(errorCallback))
    return;

  var msg = {
    'cmd': 'RegisterSinkApp',
    'datatype': dataType
  };

  postMessage(msg, function(result) {
    if (result.error) {
      if (errorCallback) {
        var error = new tizen.WebAPIError(tizen.WebAPIException.UNKNOWN_ERR);
        errorCallback(error);
      }
      return;
    }

    if (successCallback) {
      var application = new BluetoothHealthApplication(dataType, name, result);
      adapter.health_apps[result.app_id] = application;
      successCallback(application);
    }
  });
};

BluetoothHealthProfileHandler.prototype.connectToSource =
    function(peer, application, successCallback, errorCallback) {
  if (!xwalk.utils.validateArguments('oof?f', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  if (adapter.checkServiceAvailability(errorCallback))
    return;

  var msg = {
    'cmd': 'ConnectToSource',
    'address': peer.address,
    'app_id': application.app_id
  };

  postMessage(msg, function(result) {
    if (result.error != 0) {
      if (errorCallback) {
        var error = new tizen.WebAPIError(tizen.WebAPIException.UNKNOWN_ERR);
        if (result.error == 1)
          error = new tizen.WebAPIError(tizen.WebAPIException.INVALID_VALUES_ERR);
        errorCallback(error);
      }
      return;
    }

    if (successCallback) {
      var i = adapter.indexOfDevice(adapter.known_devices, result.address);
      var channel = new BluetoothHealthChannel(adapter.known_devices[i],
          adapter.health_apps[result.app_id], result);
      successCallback(channel);
    }
  });
};

function BluetoothHealthChannel(device, application, msg) {
  _addConstProperty(this, 'peer', device);
  _addConstProperty(this, 'channelType', (msg.channel_type == 1) ? 'RELIABLE' : 'STREAMING');
  _addConstProperty(this, 'application', application);
  _addConstProperty(this, 'isConnected', (msg.connected == 'true'));
  this.channel = msg.channel;
  this.data = [];
}

BluetoothHealthChannel.prototype.close = function() {
  if (adapter.checkServiceAvailability(errorCallback))
    return;

  var msg = {
    'cmd': 'DisconnectSource',
    'address': this.peer.address,
    'channel': this.channel
  };

  var channel = this;

  postMessage(msg, function(result) {
    if (result.error != 0) {
      if (errorCallback) {
        var error = new tizen.WebAPIError(tizen.WebAPIException.UNKNOWN_ERR);
        errorCallback(error);
      }
      return;
    }

    _addConstProperty(channel, 'isConnected', false);
    if (adapter.health_channel_listener.onclose)
      adapter.health_channel_listener.onclose();
  });
};

BluetoothHealthChannel.prototype.sendData = function(data) {
  if (adapter.checkServiceAvailability(errorCallback))
    return;

  if (!xwalk.utils.validateArguments('o', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  var msg = {
    'cmd': 'SendHealthData',
    'data': data,
    'channel' : this.channel
  };

  postMessage(msg, function(result) {
    if (result.error != 0) {
      var error = new tizen.WebAPIError(tizen.WebAPIException.UNKNOWN_ERR);
      return 0;
    }

    return result.size;
  });
};

BluetoothHealthChannel.prototype.setListener = function(listener) {
  if (!xwalk.utils.validateArguments('o', arguments)) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  if (!xwalk.utils.validateObject(listener, 'ff', ['onmessage', 'onclose'])) {
    throw new tizen.WebAPIException(tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  adapter.health_channel_listener = listener;
};

BluetoothHealthChannel.prototype.unsetListener = function() {
  adapter.health_channel_listener = {};
};

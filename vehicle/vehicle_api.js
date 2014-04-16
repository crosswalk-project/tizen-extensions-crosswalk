// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var next_async_call_id = 0;
var async_calls = {};

function VehicleInterface(attname) {
  this.attributeName = attname;

  var msg = {};
  msg['method'] = 'zones';
  msg['name'] = this.attributeName;

  this._zones = [];

  var call = new AsyncCall(function(data) {
    this._zones = data;
  });

  async_calls[next_async_call_id] = call;
  ++next_async_call_id;

  extension.postMessage(JSON.stringify(msg));

  Object.defineProperty(this, 'zones', { get: function() { return this._zones } });
}

VehicleInterface.prototype.get = function(zone) {
  var msg = {};
  msg['method'] = 'get';
  msg['name'] = this.attributeName;
  msg['zone'] = zone;

  return createPromise(msg);
};

function AsyncCall(resolve, reject) {
  this.resolve = resolve;
  this.reject = reject;
}

function createPromise(msg) {
  var promise = new Promise(function(resolve, reject) {
    async_calls[next_async_call_id] = new AsyncCall(resolve, reject);
  });

  msg.asyncCallId = next_async_call_id;
  extension.postMessage(JSON.stringify(msg));
  ++next_async_call_id;
  return promise;
}

function _defineVehicleProperty(obj, prop) {
  Object.defineProperty(obj, prop, { enumerable: true, value: new VehicleInterface(prop) });
}

extension.setMessageListener(function(json) {
  var msg = JSON.parse(json);

  console.log('message received: ' + msg);

  switch (msg.method) {
    case 'get':
      handleGetReply(msg);
      break;
    case 'zones':
      handleZonesReply(msg);
      break;
  }
});

function handleGetReply(msg) {
  console.log('handle get reply');

  var cbobj = async_calls[msg.asyncCallId];

  if (msg.error)
    cbobj.reject(msg.value);
  else
    cbobj.resolve(msg.value);

  delete async_calls[msg.asyncCallId];
}

function handleZonesReply(msg) {
  var cbobj = async_calls[msg.asyncCallId];

  if (cbobj)
    cbobj.resolve(msg.value);
}

_defineVehicleProperty(exports, 'vehicleSpeed');

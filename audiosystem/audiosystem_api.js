// Copyright(c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license this can be
// found in the LICENSE file.
//
//
//  Proposed Tizen API : https://wiki.tizen.org/wiki/User:Tanuk/AudioSystemAPI
//

(function() {
  var _g_context = null;
  var _callbacks = new Array(256);
  var _req_id = 0;
  var _debug_enabled = 0;  // toggle this to show/hide debug logs

  //
  // Logging helpers
  //
  function DBG(msg) {
    if (!_debug_enabled) return;
    console.log('[AudioSystem-D]:' + msg);
  }

  function ERR(msg) {
    console.error('[AudioSystem-E]:' + msg);
  }

  //
  // Helper functions
  //
  function _addConstProperty(obj, prop, propValue) {
    Object.defineProperty(obj, prop, {
      value: propValue == undefined ? null : propValue,
      writable: false,
      configurable: true,
      enumerable: true
    });
  }

  function _addConstPropertyWithGetter(obj, prop, getter) {
    Object.defineProperty(obj, prop, {
      get: getter,
      configurable: true,
      enumerable: true
    });
  }

  function _addProperty(obj, prop, propValue) {
    Object.defineProperty(obj, prop, {
      value: propValue,
      writable: true,
      configurable: true,
      enumerable: true
    });
  }

  function _getArrayIndexById(array, id) {
    for (var i = 0, len = array.length; i < len; i++) {
      if (array[i].id == id) return i;
    }

    return -1;
  }

  function _throwTypeMismatch(msg) {
    throw new tizen.WebAPIException(
        tizen.WebAPIException.TYPE_MISMATCH_ERR);
  }

  function _dispatchEvent(obj, type, args) {
    var ev_args = args || { };
    var ev = new CustomEvent(type);
    for (var key in ev_args)
      _addConstProperty(ev, key, ev_args[key]);

    obj.dispatchEvent(ev);

    // call EventHandler attached, if any
    var handler = obj['on' + type];
    if (handler !== null && typeof handler === 'function') {
      handler(ev);
    }
  }

  function _handleContextDisconnected() {
    _dispatchEvent(exports, 'disconnected');
  }

  function _handleContextChanges(changes) {
    var change_handlers = {
      main_input_volume_control: function(value) {
        _g_context._main_vc_in = value;
      },
      main_output_volume_control: function(value) {
        _g_context._main_vc_out = value;
      },
      main_input_mute_control: function(value) {
        _g_context._main_mc_in = value;
      },
      main_output_mute_control: function(value) {
        _g_context._main_mc_out = value;
      }
    };
    for (var type in changes) {
      change_handlers[type](changes[type]);
      var ev = type.replace(/_/g, '') + 'changed';
      _dispatchEvent(_g_context, ev);
    }
  }

  function _handleVolumeControlAdded(ctrl) {
    DBG('volume control added #' + ctrl.id + ' ' + ctrl.label);
    var vc = new VolumeControl(ctrl);

    _g_context.volume_controls.push(vc);
    _dispatchEvent(_g_context, 'volumecontroladded', { 'control': vc });
  }

  function _handleMuteControlAdded(ctrl) {
    DBG('mute control added #' + ctrl.id + ' ' + ctrl.label);

    var mc = new MuteControl(ctrl);

    _g_context.mute_controls.push(mc);
    _dispatchEvent(_g_context, 'mutecontroladded', { 'control': mc });
  }

  function _handleAudioDeviceAdded(dev) {
    DBG('audio device added #' + dev.id + ' ' + dev.label);

    var device = new AudioDevice(dev);

    _g_context.devices.push(device);
    _dispatchEvent(_g_context, 'deviceadded', { 'device' : device });
  }

  function _handleAudioGroupAdded(grp) {
    DBG('audio group added #' + grp.id + ' ' + grp.label);

    var group = new AudioGroup(grp);

    _g_context.audio_groups.push(group);
    _dispatchEvent(_g_context, 'audiogroupadded', { 'group': group });
  }

  function _handleAudioStreamAdded(as) {
    DBG('audio stream added #' + as.id + ' ' + as.label);

    var stream = new AudioStream(as);

    _g_context.streams.push(stream);
    _dispatchEvent(_g_context, 'streamadded', { 'stream': stream });
  }

  function _handleVolumeControlRemoved(id) {
    DBG('volume control removed #' + id);
    var index = _getArrayIndexById(_g_context.volume_controls, id);

    if (index == -1)
      return;
    var vc = _g_context.volume_controls.splice(index, 1);
    _dispatchEvent(_g_context, 'volumecontrolremoved', { 'control': vc[0] });
  }

  function _handleMuteControlRemoved(id) {
    DBG('mute control removed #' + id);
    var index = _getArrayIndexById(_g_context.mute_controls, id);

    if (index == -1)
      return;
    var mc = _g_context.mute_controls.splice(index, 1);
    _dispatchEvent(_g_context, 'mutecontrolremoved', { 'control': mc[0] });
  }

  function _handleAudioDeviceRemoved(id) {
    DBG('audio device removed #' + id);
    var index = _getArrayIndexById(_g_context.devices, id);

    if (index == -1)
      return;
    var d = _g_context.devices.splice(index, 1);
    _dispatchEvent(_g_context, 'deivceremoved', { 'device': d[0] });
  }

  function _handleAudioGroupRemoved(id) {
    DBG('audio group removed #' + id);
    var index = _getArrayIndexById(_g_context.audio_groups, id);

    if (index == -1)
      return;
    var g = _g_context.audio_groups.splice(index, 1);
    _dispatchEvent(_g_context, 'audiogroupremoved', { 'group': g[0] });
  }

  function _handleAudioStreamRemoved(id) {
    DBG('audio stream removed #' + id);
    var index = _getArrayIndexById(_g_context.streams, id);

    if (index == -1)
      return;
    var s = _g_context.streams.splice(index, 1);
    _dispatchEvent(_g_context, 'streamremoved', { 'stream': s[0] });
  }

  function _changeEventHelper(obj, new_obj, changes) {
    changes.forEach(function(prop_name) {
      _addConstProperty(obj, prop_name, new_obj[prop_name]);
      var event_prefix = prop_name.replace('_', '');
      _dispatchEvent(obj, event_prefix + 'changed');
    });
  }

  function _handleVolumeControlChanges(new_vc, changes) {
    DBG('volume control changed #' + new_vc.id);
    var index = _getArrayIndexById(_g_context.volume_controls, new_vc.id);
    if (index == -1) {
      ERR('No volume control found #' + new_vc.id);
      return;
    }
    _changeEventHelper(_g_context.volume_controls[index], new_vc, changes);
  }

  function _handleMuteControlChanges(new_mc, changes) {
    DBG('mute control changed #' + new_mc.id);
    var index = _getArrayIndexById(_g_context.mute_controls, new_mc.id);
    if (index == -1) {
      ERR('No mute control found #' + new_mc.id);
      return;
    }
    _changeEventHelper(_g_context.mute_controls[index], new_mc, changes);
  }

  function _handleAudioDeviceChanges(new_ad, changes) {
    DBG('audio device chagned');
    var index = _getArrayIndexById(_g_context.devices, new_ad.id);
    if (index == -1) {
      ERR('No device found #' + new_ad.di);
      return;
    }
    _changeEventHelper(_g_context.devices[index], new_ad, changes);
  }

  function _handleAudioGroupChanges(new_ag, changes) {
    DBG('audio group changed #' + new_ag.id);
    var index = _getArrayIndexById(_g_context.audio_groups, new_ag.id);
    if (index == -1) {
      ERR('No audio group found #' + new_ag.id);
      return;
    }
    _changeEventHelper(_g_context.audio_groups[index], new_ag, changes);
  }

  function _handleAudioStreamChanges(new_as, changes) {
    DBG('audio stream changed: ' + new_as.id);
    var index = _getArrayIndexById(_g_context.streams, new_as.id);
    if (index == -1) {
      ERR('No audio stream found #' + new_as.id);
      return;
    }
    _changeEventHelper(_g_context.streams[index], new_as, changes);
  }

  function _handleSync() {
    _dispatchEvent(_g_context, 'sync');
  }

  function _getNextReqId() {
    // find the next available request number
    for (var i = 0, max = _callbacks.length; i < max; i++) {
      if (_callbacks[i] == null || _callbacks[i] == undefined)
        return i;
    }
    return i;
  }

  //
  // Extension message listener
  //
  extension.setMessageListener(function(json) {
    DBG('Message Listener : \n' + json + '\n');
    var msg = JSON.parse(json);
    var req_id = msg.req_id;

    // if req_id found, means this is reaply to earlier request
    if (req_id != null) {
      var cb = _callbacks[req_id];
      delete msg.req_id;

      if (!cb) {
        ERR('Invalid req_id #' + req_id);
      } else {
        _callbacks[req_id] = null;
        cb(msg);
      }
      return;
    }

    // its a signal from backend
    switch (msg.cmd) {
      case 'disconnected':
        _handleContextDisconnected();
        break;
      case 'context_changed':
        _handleContextChanges(msg.changes);
        break;
      case 'volume_control_added':
        _handleVolumeControlAdded(msg.control);
        break;
      case 'mute_control_added':
        _handleMuteControlAdded(msg.control);
        break;
      case 'device_added':
        _handleAudioDeviceAdded(msg.device);
        break;
      case 'group_added':
        _handleAudioGroupAdded(msg.group);
        break;
      case 'stream_added':
        _handleAudioStreamAdded(msg.stream);
        break;

      case 'volume_control_removed':
        _handleVolumeControlRemoved(msg.id);
        break;
      case 'mute_control_removed':
        _handleMuteControlRemoved(msg.id);
        break;
      case 'device_removed':
        _handleAudioDeviceRemoved(msg.id);
        break;
      case 'group_removed':
        _handleAudioGroupRemoved(msg.id);
        break;
      case 'stream_removed':
        _handleAudioStreamRemoved(msg.id);
        break;

      case 'volume_control_changed':
        _handleVolumeControlChanges(msg.control, msg.changes);
        break;
      case 'mute_control_changed':
        _handleMuteControlChanges(msg.control, msg.changes);
        break;
      case 'group_changed':
        _handleAudioGroupChanges(msg.group, msg.changes);
        break;
      case 'device_changed':
        _handleAudioDeviceChanges(msg.device, msg.changes);
        break;
      case 'stream_changed':
        _handleAudioStreamChanges(msg.stream, msg.changes);
        break;
      case 'sync':
        _handleSync();
        break;
      default:
        ERR("Unknown signal '" + msg.cmd + "' from backend");
    }
  });

  //
  // post asynchronous message to extension
  //
  function _postMessage(command, args, cb) {
    try {
      var msg = args || {};

      msg.req_id = _getNextReqId();
      msg.cmd = command;

      _callbacks[msg.req_id] = cb;

      extension.postMessage(JSON.stringify(msg));
    } catch (e) {
      ERR('PostMessage Exception : ' + e);
    }
  }

  //
  // sends synchronus message to extension
  //
  function _sendMessage(command, args) {
    try {
      var _msg = args || {};
      _msg.cmd = command;

      var serialized = JSON.stringify(_msg);

      DBG('Sending Message : ' + serialized);
      var reply = extension.internal.sendSyncMessage(serialized);
      DBG('Got Reply : ' + reply);

      return JSON.parse(reply);
    } catch (e) {
      ERR('Exception: ' + e);
    }
  }

  function EventTarget(event_types) {
    var _event_listeners = { };
    var _event_handlers = { };
    var self = this;

    if (event_types != null) {
      // initialize event listeners
      event_types.forEach(function(type) {
        _event_listeners[type] = [];
        // Setup a Event Handler
        _addProperty(self, 'on' + type, null);
      });
    }

    this._event_listeners = _event_listeners;
  }

  EventTarget.prototype.isValidEventType = function(type) {
    return type in this._event_listeners;
  };

  EventTarget.prototype.addEventListener = function(type, listener) {
    if (this.isValidEventType(type) &&
        listener != null && typeof listener === 'function' &&
        this._event_listeners[type].indexOf(listener) == -1) {
      this._event_listeners[type].push(listener);
      return;
    }
    ERR('Invalid event type \'' + type + '\', ignoring.' +
        'avaliable event types : ' + this._event_listeners);
  };

  EventTarget.prototype.removeEventListener = function(type, listener) {
    if (!type || !listener)
      return;

    var index = this._event_listeners[type].indexOf(listener);
    if (index == -1)
      return;

    this._event_listeners[type].splice(index, 1);
  };

  EventTarget.prototype.dispatchEvent = function(ev) {
    var handled = true;
    var listeners = null;
    if (typeof ev === 'object' &&
        this.isValidEventType(ev.type) &&
        (listeners = this._event_listeners[ev.type]) != null) {
      listeners.forEach(function(listener) {
        if (!listener(ev) && handled)
          handled = false;
      });
    }
    return handled;
  };

  //
  // Base Object for all audio system objects
  //
  function AudioSystemBaseObject(id, label, events) {
    events.push('labelchanged');
    EventTarget.call(this, events);

    _addConstProperty(this, 'id', id);
    _addConstProperty(this, 'label', label);
  }
  AudioSystemBaseObject.prototype = Object.create(EventTarget.prototype);

  function _postMessageHelper(cmd, args, success_cb, error_cb, error_hint) {
    _postMessage(cmd, args, function(reply) {
      if (reply.error) {
        ERR(error_hint + reply.errorMsg);
        if (error_cb) error_cb(new tizen.WebAPIError(reply.errorMsg));
        return;
      }
      if (success_cb) success_cb();
    });
  }

  //
  // interface VolumeControl
  //
  function VolumeControl(ctrl) {
    AudioSystemBaseObject.call(this, ctrl.id, ctrl.label,
        ['volumechanged', 'volumestepschanged', 'balancechanged']);

    DBG('Volume Control: #' + ctrl.id + ' ' + ctrl.label);
    _addConstProperty(this, 'volume', ctrl.volume);
    _addConstProperty(this, 'balance', ctrl.balance);
    _addConstProperty(this, 'volume_steps', ctrl.volume_steps);
    if (ctrl.balance.length != 0) {
      ctrl.balance.forEach(function(b) {
        // channel position converison: ex: front-left -> front_left
        b.position = b.position.replace(/-/g, '_');
      });
    }

    this.set_volume = function(volume, success_cb, error_cb) {
      if (!xwalk.utils.validateArguments('n?ff', arguments))
        _throwTypeMismatch();

      _postMessageHelper('setVolume', {
        'id' : this.id,
        'volume': volume
      }, success_cb, error_cb, 'Setting volume:');
    };

    this.set_balance = function(balance, success_cb, error_cb) {
      if (!xwalk.utils.validateArguments('a?ff', arguments))
        _throwTypeMismatch();

      _postMessageHelper('setBalance', {
        'id' : this.id,
        'balance' : balance
      }, success_cb, error_cb, 'Setting balance:');
    };

    this.set_simplified_balance = function(s_balance, success_cb, error_cb) {
      if (!xwalk.utils.validateArguments('o?ff', arguments)) {
        _throwTypeMismatch();
      }
      if (!xwalk.utils.validateArguments('n', s_balance.balance) ||
          !xwalk.utils.validateArguments('n', s_balance.fade)) {
        _throwTypeMismatch();
      }
      _postMessageHelper('setSimplifiedBalance', {
        'id': this.id,
        'balance' : s_balance.balance,
        'fade': s_balance.fade
      }, success_cb, error_cb, 'Setting simplified balance:');
    };

    this.get_simplified_balance = function() {
      var reply = _sendMessage('getSimplifiedBalance', { 'id': this.id });

      if (reply.error) {
        throw tizen.WebAPIException(reply.errorMsg);
      }

      return {'balance': reply.balance, 'fade': reply.fade};
    };
  }
  VolumeControl.prototype = Object.create(AudioSystemBaseObject.prototype);

  //
  // interface MuteControl
  //
  function MuteControl(ctrl) {
    AudioSystemBaseObject.call(this, ctrl.id, ctrl.label, ['mutedchanged']);

    DBG('Mute Control #' + ctrl.id + ' ' + ctrl.label);

    _addConstProperty(this, 'muted', ctrl.muted);

    this.set_muted = function(muted, success_cb, error_cb) {
      if (!xwalk.utils.validateArguments('b?ff', arguments)) {
        _throwTypeMismatch();
      }

      _postMessageHelper('setMuted', {
        'id': this.id,
        'muted': muted
      }, success_cb, error_cb, 'Failed to set muted:');
    };
  }
  MuteControl.prototype = Object.create(AudioSystemBaseObject.prototype);

  //
  // interface AudioDevice
  //
  function AudioDevice(dev) {
    AudioSystemBaseObject.call(this, dev.id, dev.label,
        ['devicetypeschanged', 'volumecontrolchanged', 'mutecontrolchanged']);

    DBG('Audio Device #' + dev.id + ' ' + dev.label);

    _addConstProperty(this, 'direction', dev.direction);
    _addConstProperty(this, 'device_types', dev.device_types);
    _addConstPropertyWithGetter(this, 'volume_control', function() {
      if (this._vc == null) return null;
      var i = _getArrayIndexById(_g_context.volume_controls, this._vc);
      return i != -1 ? _g_context.volume_controls[i] : null;
    });
    _addConstPropertyWithGetter(this, 'mute_control', function() {
      if (this._mc == null) return null;
      var i = _getArrayIndexById(_g_context.mute_controls, this._mc);
      return i != -1 ? _g_context.mute_controls[i] : null;
    });

    // private data
    this._vc = 'volume_control' in dev ? dev.volume_control : null;
    this._mc = 'mute_control' in dev ? dev.mute_control : null;
  }
  AudioDevice.prototype = Object.create(AudioSystemBaseObject.prototype);

  //
  // interface AudioStream
  //
  function AudioStream(stream) {
    AudioSystemBaseObject.call(this, stream.id, stream.label,
        ['volumecontrolchanged', 'mutecontrolchanged']);

    DBG('Audio Stream #' + stream.id + ' ' + stream.label);

    _addConstProperty(this, 'direction', stream.direction);

    _addConstPropertyWithGetter(this, 'volume_control', function() {
      if (this._vc == null) return null;
      var i = _getArrayIndexById(_g_context.volume_controls, this._vc);
      return i != -1 ? _g_context.volume_controls[i] : null;
    });
    _addConstPropertyWithGetter(this, 'mute_control', function() {
      if (this._mc == null) return null;
      var i = _getArrayIndexById(_g_context.mute_controls, this._mc);
      return i != -1 ? _g_context.mute_controls[i] : null;
    });

    // private data
    this._vc = 'volume_control' in stream ? stream.volume_control : null;
    this._mc = 'mute_control' in stream ? stream.mute_control : null;
  }
  AudioStream.prototype = Object.create(AudioSystemBaseObject.prototype);

  //
  // interface AudioGroup
  //
  function AudioGroup(grp) {
    AudioSystemBaseObject.call(this, grp.id, grp.label,
        ['volumecontrolchanged', 'mutecontrolchanged']);

    DBG('Audio Group #' + grp.id + ' ' + grp.label);

    _addConstProperty(this, 'id', grp.id);
    _addConstProperty(this, 'label', grp.label);
    _addConstProperty(this, 'name', grp.name);
    _addConstPropertyWithGetter(this, 'volume_control', function() {
      if (this._vc == null) return null;
      var i = _getArrayIndexById(_g_context.volume_controls, this._vc);
      return i != -1 ? _g_context.volume_controls[i] : null;
    });
    _addConstPropertyWithGetter(this, 'mute_control', function() {
      if (this._mc == null) return null;
      var i = _getArrayIndexById(_g_context.mute_controls, this._mc);
      return i != -1 ? _g_context.mute_controls[i] : null;
    });

    // private data
    this._vc = 'volume_control' in grp ? grp.volume_control : null;
    this._mc = 'mute_control' in grp ? grp.mute_control : null;
  }
  AudioGroup.prototype = Object.create(AudioSystemBaseObject.prototype);

  //
  // interface AudioSystemContext
  //
  function AudioSystemContext(ctx) {
    var vc_array = [];
    var mc_array = [];
    var ag_array = [];
    var ad_array = [];
    var as_array = [];

    AudioSystemBaseObject.call(this, ctx.id, ctx.label, [
      'audiogroupupdated',
      'deviceadded',
      'deviceremoved',
      'mainoutputvolumecontrolchanged',
      'maininputvolumecontrolchanged',
      'mutecontroladded',
      'mutecontrolremoved',
      'mainoutputmutecontrolchanged',
      'maininputmutecontrolchanged',
      'onaudiogroupremoved',
      'streamadded',
      'streamremoved',
      'sync',
      'volumecontroladded',
      'volumecontrolremoved']);

    // privte data
    this._main_vc_out = 'main_output_volume_control' in ctx ?
                        ctx.main_output_volume_control : null;
    this._main_vc_in = 'main_input_volume_control' in ctx ?
                       ctx.main_input_volume_control : null;
    this._main_mc_out = 'main_output_mute_control' in ctx ?
                        ctx.main_output_mute_control : null;
    this._main_mc_in = 'main_input_mute_control' in ctx ?
                       ctx.main_input_mute_control : null;

    ctx.volume_controls.forEach(function(ctrl) {
      vc_array.push(new VolumeControl(ctrl));
    });

    ctx.mute_controls.forEach(function(ctrl) {
      mc_array.push(new MuteControl(ctrl));
    });

    ctx.audio_groups.forEach(function(grp) {
      ag_array.push(new AudioGroup(grp));
    });

    ctx.devices.forEach(function(dev) {
      ad_array.push(new AudioDevice(dev));
    });

    ctx.streams.forEach(function(stream) {
      as_array.push(new AudioStream(stream));
    });

    _addConstProperty(this, 'volume_controls', vc_array);
    _addConstProperty(this, 'mute_controls', mc_array);
    _addConstProperty(this, 'devices', ad_array);
    _addConstProperty(this, 'streams', as_array);
    _addConstProperty(this, 'audio_groups', ag_array);
    _addConstPropertyWithGetter(this, 'main_output_volume_control', function() {
      if (this._main_vc_out == null) return null;
      var i = _getArrayIndexById(this.volume_controls, this._main_vc_out);
      return i != -1 ? this.volume_controls[i] : null;
    });
    _addConstPropertyWithGetter(this, 'main_input_volume_control', function() {
      if (this._main_vc_in == null) return null;
      var i = _getArrayIndexById(this.volume_controls, this._main_vc_in);
      return i != -1 ? this.volume_controls[i] : null;
    });
    _addConstPropertyWithGetter(this, 'main_output_mute_control', function() {
      if (this._main_mc_out == null) return null;
      var i = _getArrayIndexById(this.mute_controls, this._main_mc_out);
      return i != -1 ? this.mute_controls[i] : null;
    });
    _addConstPropertyWithGetter(this, 'main_input_mute_control', function() {
      if (this._main_mc_in == null) return null;
      var i = _getArrayIndexById(this.mute_controls, this._main_mc_in);
      return i != -1 ? this.mute_controls[i] : null;
    });

    vc_array = mc_array = ad_array = as_array = ag_array = null;
  }
  AudioSystemContext.prototype = Object.create(AudioSystemBaseObject.prototype);

  //
  //  interface AudioSystem
  //
  function AudioSystem() {
    EventTarget.call(this, ['disconnected']);

    this.connect = function(success_cb, error_cb) {
      if (!xwalk.utils.validateArguments('?ff', arguments))
        _throwTypeMismatch();

      if (_g_context != null) {
        if (success_cb)
          success_cb(_g_context);
        return;
      }
      _postMessage('connect', null, function(reply) {
        if (reply.error) {
          ERR('Failed to connect.' + reply.errorMsg);
          if (error_cb) error_cb(reply.errorMsg);
          return;
        }
        _g_context = new AudioSystemContext(reply.context);
        if (success_cb) success_cb(_g_context);
      });
    };

    this.disconnect = function() {
      if (_g_context == null) return;

      _postMessage('disconnect', null, function() {
        DBG('Disconnected');
        _g_context = null;
      });
    };
  }
  AudioSystem.prototype = Object.create(EventTarget.prototype);

  // Export main entry point for AudioSystem API
  exports = new AudioSystem();
})();

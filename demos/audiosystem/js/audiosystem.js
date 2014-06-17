/*
Copyright (c) 2014 Intel Corporation. All rights reserved.
Use of this source code is governed by a MIT-style license that can be
found in the LICENSE file.
*/
$(function() {
  Array.prototype.findById = function(id) {
    for (var i = 0; i < this.length; i++) {
      if (this[i].id == id) return this[i];
    }
    return null;
  };

  function updateVolume($table, vc) {
    if (vc) {
      var volume = (vc.volume.toFixed(2) * 100).toFixed(0);
      var $slider = $table.find('#slider');
      /* Volume */
      $slider.slider({
        value: volume,
        range: 'min',
        disabled: false,
        create: function(ev, ui) {
          $(this).find('a')
                 .prop('href', null)
                 .css({
                    'width': '2.0em',
                    'height': '1.5em',
                    'font-size': '10px',
                    'text-align': 'center'
                  }).html(volume);
        },
        change: function(ev, ui) {
          $slider.find('a').html(ui.value);
        },
        slide: function(ev, ui) {
          vc.set_volume(ui.value / 100);
        }
      });

      $table.find('tr[id=volume]').remove();
      $table.find('tr:last').show();
      if (vc.balance.length > 1) {
        /* per channel balance */
        vc.balance.forEach(function(b, index, b_array) {
          var balance = b.balance.toFixed(2) * 100;
          var $label = $('<label>').attr('class', 'label').html(b.label + ':');

          var $slider = $('<div min=0 max=100>').slider({
            id: index,
            range: 'min',
            value: balance,
            create: function(ev, ui) {
              $(this).find('a')
                     .prop('href', null)
                     .css({
                        'width': '1.6em',
                        'height': '1.5em',
                        'font-size': '10px',
                        'text-align': 'center'
                      }).html(balance);
            },
            change: function(ev, ui) {
              $slider.find('a').html(ui.value);
            },
            slide: function(ev, ui) {
              var length = vc.balance.length;
              var b = [];
              for (var i = 0; i < length; i++) {
                b[i] = vc.balance[i].balance;
              }
              b[index] = ui.value / 100;

              vc.set_balance(b, function() {}, function(err) {
                console.log('set_balance: FAILED: ' + err);
              });
            }
          });

          var $row = $('<tr id=volume>');
          $row.append($('<td>').html($label))
              .append($('<td>').html($slider));

          $table.find('tbody').append($row);
        });
      }

      vc.addEventListener('volumechanged', function() {
        $slider.slider('option', 'value', vc.volume.toFixed(2) * 100);
      });

      vc.addEventListener('balancechanged', function() {
        vc.balance.forEach(function(b, index) {
          var $s = $table.find('div[id=' + index + ']');
          $s.slider('option', 'value', b.balance);
        });
      });

      vc.addEventListener('volumestepschanged', function() {
        console.log('Ignoring volume steps change');
      });

      vc.addEventListener('labelchanged', function() {
        console.log('Ignoring volume control label change');
      });
    } else {
      $table.find('tr[id=volume]').remove();
      $table.find('tr:last').hide();
    }
  }

  function updateMute($chkbox, mc) {
    if (mc) {
      $chkbox.prop({
        checked: mc.muted,
        disabled: false
      });
      $chkbox.on('change', function() {
        try {
          mc.set_muted(Boolean($chkbox.prop('checked')));
        } catch (e) {
          console.log('Exp : ' + e.name);
        }
      });

      mc.addEventListener('mutedchanged', function() {
        $chkbox.prop('checked', mc.muted);
      });

      mc.addEventListener('labelchanged', function() {
        console.log('Ignoring mute control label change');
      });
    } else {
      $chkbox.prop('disabled', true);
    }
  }

  /* Volume/Mute Control */
  function showMainControl($parent, vc, mc) {
    if (vc == null) {
      $parent.find('table').hide();
      $parent.find('.info').show();
      return;
    }

    var $slider = $parent.find('#slider');
    var $chkbox = $parent.find('#mute');

    if (vc != false) {
      updateVolume($parent.find('table'), vc);
      $parent.find('#label').html(vc.label);
      vc.addEventListener('labelchanged', function() {
        $parent.find('#label').html(vc.label);
      });
    }

    if (mc != false) updateMute($chkbox, mc);
  }

  function showMainVolumeControl($parent, ctx) {
    $input = $parent.find('#main-input');
    $output = $parent.find('#main-output');

    showMainControl($output, ctx.main_output_volume_control,
        ctx.main_output_mute_control);
    showMainControl($input, ctx.main_input_volume_control,
        ctx.main_input_mute_control);

    ctx.onmainoutputvolumecontrolchanged = function() {
      console.log('main OUTPUT VOLUME control changed : #' +
          ctx.main_output_volume_control);
      showMainControl($output, ctx.main_output_volume_control, false);
    };
    ctx.onmainoutputmutecontrolchanged = function() {
      console.log('main OUTPUT MUTE control changed : #' +
          ctx.main_output_mute_control);
      showMainControl($output, false, ctx.main_output_mute_control);
    };

    ctx.onmaininputvolumecontrolchanged = function() {
      console.log('main INPUT VOLUME control change: #' +
          ctx.main_input_volume_control);
      showMainControl($input, ctx.main_input_volume_control, false);
    };
    ctx.onmaininputmutecontrolchagned = function() {
      console.log('main INPIT MUTE control changed: #' +
          ctx.main_input_mute_control);
      showMainControl($input, false, ctx.main_input_mute_control);
    };
  }

  function showVolumeControls($parent, ctx) {
    var $select = $parent.find('#controls');
    var $table = $parent.find('table');

    function addControl(vc) {
      $('<option>').attr('value', vc.id).html(vc.label).appendTo($select);
    }
    ctx.onvolumecontroladded = function(ev) {
      addControl(ev.control);
      if (ctx.volume_controls.length == 1) {
        $parent.find('.info').hide();
        $table.show();
      }
    };
    ctx.onvolumecontrolremvoed = function(ev) {
      $select.find('option[value=' + ev.control.id + ']').remove();
      if (ctx.volume_controls.length == 0) {
        $parent.find('.info').show();
        $table.hide();
      }
    };

    function findControlByName(controls, label) {
      for (var i = 0; i < controls.length; i++) {
        if (controls[i].label == label) return controls[i];
      }
      return null;
    }

    $select.on('change', function() {
      var vc = ctx.volume_controls.findById($select.prop('value'));

      updateVolume($table, vc);

      var mc = findControlByName(ctx.mute_controls, vc.label);
      updateMute($table.find('#mute'), mc);
    });

    if (ctx.volume_controls.length == 0) {
      $parent.find('.info').show();
      $table.hide();
    } else {
      ctx.volume_controls.forEach(addControl);

      var ev = document.createEvent('Event');
      ev.initEvent('change', true, false);
      $select.get()[0].dispatchEvent(ev);
    }
  }

  function showAudioDevices($parent, ctx) {
    var $out_select = $parent.find('#output #out_devices');
    var $in_select = $parent.find('#input #in_devices');

    function addDevice(d) {
      var $opt = $('<option>').attr('value', d.id).html(d.label);

      if (d.direction == 'input')
        $opt.appendTo($in_select);
      else if (d.direction == 'output')
        $opt.appendTo($out_select);
      else if (d.direction == 'bidirectional') {
        $opt.clone().appendTo($in_select);
        $opt.appendTo($out_select);
      }
    }

    ctx.ondeviceadded = function(ev) {
      addDevice(ev.device);
    };

    ctx.ondeviceremoved = function(ev) {
      var d = ev.device;

      if (d.direction == 'output')
        $out_select.find('option[value=' + d.id + ']').remove();
      else if (ad.direction == 'input')
        $in_select.find('option[value=' + d.id + ']').remove();
      else if (ad.direction == 'bidirectional') {
        $out_select.find('option[value=' + d.id + ']').remove();
        $in_select.find('option[value=' + d.id + ']').remove();
      }
    };

    [$out_select, $in_select].forEach(function($select) {
      $select.on('change', function() {
        var $select = $(this);
        var $table = $select.parents('table');
        var $slider = $table.find('.slider');
        var $types = $table.find('#types');
        var $chkbox = $table.find('#mute');
        var ad = ctx.devices.findById($select.prop('value'));

        $types.html(ad.device_types.length != 0 ?
            ad.device_types.join() : 'None');

        updateVolume($table, ad.volume_control);
        updateMute($chkbox, ad.mute_control);

        ad.onlabelchanged = function() {
          $select.find('option[value=' + ad.id + ']').html(ad.label);
        };

        ad.ondevicetypeschanged = function() {
          $types.html(ad.device_types.length != 0 ?
              ad.device_types.join() : 'None');
        };

        ad.onvolumecontrolchanged = function() {
          updateVolume($table, ad.volume_control);
        };

        ad.onmutecontrolchanged = function() {
          updateMute($chkbox, ad.mute_control);
        };
      });
    });

    ctx.devices.forEach(addDevice);

    var ev = document.createEvent('Event');
    ev.initEvent('change', true, false);
    $out_select.get()[0].dispatchEvent(ev);
    $in_select.get()[0].dispatchEvent(ev);
  }

  /* Audio Streams */
  function showAudioStreams($parent, ctx) {
    var $select = $parent.find('#streams');

    function appendStream(s) {
      $select.append(
          '<option value=' + s.id + '>' + s.label + '</option>');
    }

    ctx.onstreamadded = function(ev) {
      appendStream(ev.stream);

      if (ctx.streams.length == 1) {
        $parent.find('.info').hide();
        $parent.find('table').show();

        var ev = document.createEvent('Event');
        ev.initEvent('change', true, false);
        $select.get()[0].dispatchEvent(ev);
      }
    };

    ctx.onstreamremoved = function(ev) {
      $select.find('option[value=' + ev.stream.id + ']').remove();

      if (ctx.streams.length == 0) {
        $parent.find('.info').show();
        $parent.find('table').hide();
      }
    };

    $select.on('change', function() {
      var $slider = $parent.find('.slider');
      var $chkbox = $parent.find('#mute');
      var id = $select.prop('value');
      var as = ctx.streams.findById(id);

      $parent.find('#direction').html(as.direction);

      updateVolume($parent.find('table'), as.volume_control);
      updateMute($chkbox, as.mute_control);

      as.onlabelchanged = function() {
        $select.find('option[value=' + as.id + ']').html(as.label);
      };

      as.onvolumecontrolchanged = function() {
        updateVolume($parent.find('table'), as.volume_control);
      };

      as.onmutecontrolchanged = function() {
        updateMute($chkbox, as.mute_control);
      };
    });

    if (ctx.streams.length == 0) {
      $parent.find('.info').show();
      $parent.find('table').hide();
    }
    else {
      ctx.streams.forEach(appendStream);

      /* dummy event to select first stream */
      var ev = document.createEvent('Event');
      ev.initEvent('change', true, false);
      $select.get()[0].dispatchEvent(ev);
    }
  }

  /* AudioGroup */
  function showAudioGroups($parent, ctx) {
    var $select = $parent.find('#groups');
    var $slider = $parent.find('.slider');
    var $chkbox = $parent.find('#mute');
    var $table = $parent.find('table');

    function addGroup(ag) {
      $('<option>').attr('value', ag.id).html(ag.name).appendTo($select);
    }

    ctx.onaudiogroupadded = function(ev) {
      addGroup(ev.group);
      if (ctx.audio_groups.length() == 1) {
        $parent.find('.info').hide();
        $table.show();

        var ev = document.createEvent('Event');
        ev.initEvent('change', true, false);
        $select.get()[0].dispatchEvent(ev);
      }
    };

    ctx.onaudiogroupremoved = function(ev) {
      $select.find('option[value=' + ev.group.id + ']').remove();
      if (ctx.audio_groups.length() == 0) {
        $parent.find('.info').show();
        $table.hide();
      }
    };

    $select.on('change', function() {
      var id = $select.prop('value');
      var ag = ctx.audio_groups.findById(id);

      updateVolume($table, ag.volume_control);
      updateMute($chkbox, ag.mute_control);

      ag.onlablechanged = function() {
        $select.find('option[value=' + ag.id + ']').html(ag.name);
      };

      ag.onvolumecontrolchanged = function() {
        updateVolume($table, ag.volume_control);
      };

      ag.onmutecontrolchanged = function() {
        updateMute($chkbox, ag.mute_control);
      };
    });

    ctx.audio_groups.forEach(addGroup);

    if (ctx.audio_groups.length == 0) {
      $parent.find('.info').show();
      $parent.find('table').hide();
    } else {
      var ev = document.createEvent('Event');
      ev.initEvent('change', true, false);
      $select.get()[0].dispatchEvent(ev);
    }
  }

  /* Connect */
  tizen.audiosystem.connect(function(context) {
    function expand() {
      var $main_controls_tab = $('#main-controls');
      var $volume_controls_tab = $('#volume-controls');
      var $audio_groups_tab = $('#audio-groups');
      var $devices_tab = $('#devices');
      var $streams_tab = $('#audio-streams');

      $('#tabs').tabs().attr('style.display', true);
      $('body #basic-view').hide();

      showMainVolumeControl($main_controls_tab, context);
      showVolumeControls($volume_controls_tab, context);
      showAudioDevices($devices_tab, context);
      showAudioStreams($streams_tab, context);
      showAudioGroups($audio_groups_tab, context);

      context.onsync = function() {
        console.log('SYNC event received');
      };

      $main_controls_tab.accordion({
        heightStyle: 'content',
        fillSpace: true
      });
      $volume_controls_tab.accordion({
        header: '',
        heightStyle: 'content',
        fillSpace: true
      });
      $devices_tab.accordion({
        heightStyle: 'content'
      });
      $streams_tab.accordion({
        header: '',
        heightStyle: 'content'
      });
      $audio_groups_tab.accordion({
        header: '',
        heightStyle: 'content',
        autoHeight: false
      });
    }

    function shrink() {
      $('#tabs').tabs().attr('style.display', false);
      $('body #basic-view').show();
    }

    expand();
  }, function(err) {
    console.log('ERROR : failed to connect ' + err.name);
    $('body .info').show();
    $('#tabs').hide();
  });
});


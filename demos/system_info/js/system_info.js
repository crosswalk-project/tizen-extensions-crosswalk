/**********************************************************
 *
 * @module: system_info.js
 * @version: 1.0
 * @copyright: © 2013 Intel
 * 
 **********************************************************/

function BatteryInfo() {
  this.level = $('<p></p>').addClass('center');
  this.charging = $('<p></p>').addClass('center');
  $('#battery_data').append(this.level, this.charging);

  this.draw_graph = function draw_graph(BATDATA) {
    $('#battery_container div span').css('width', BATDATA.level * 100 + '%');
    if (BATDATA.isCharging) {
      $('#battery_container div').addClass('shine');
    } else {
      $('#battery_container div').removeClass('shine');
    }
  };

  this.getInfo = function getInfo(BATTERY) {
    tizen.systeminfo.getPropertyValue(
      "BATTERY",
      function(battery) {
        BATTERY.draw_graph(battery);
        BATTERY.charging.text(battery.isCharging ? 'Charging':'No Charging');
        BATTERY.level.text(battery.level * 100 + '%');
      },
      null);
    tizen.systeminfo.addPropertyValueChangeListener(
      "BATTERY",
      function(battery) {
        BATTERY.draw_graph(battery);
        BATTERY.charging.text(battery.isCharging ? 'Charging':'No Charging');
        BATTERY.level.text(battery.level * 100 + '%');
      });
  };
}

function BuildInfo() {
  this.model = $('<p></p>').addClass('center');
  this.manufacturer = $('<p></p>').addClass('center');
  this.build_version = $('<p></p>').addClass('center');
  $('#build').append(this.model, this.manufacturer, this.build_version);

  this.getInfo = function getInfo(BUILD) {
    tizen.systeminfo.getPropertyValue(
      "BUILD",
      function(build) {
        BUILD.model.text("MODEL: " + build.model);
        BUILD.manufacturer.text("MANUFACTURE: " + build.manufacturer);
        BUILD.build_version.text("BUILDVERSION: " + build.buildVersion);
      },
      null);
    tizen.systeminfo.addPropertyValueChangeListener(
      "BUILD",
      function(build) {
        BUILD.model.text("MODEL: " + build.model);
        BUILD.manufacturer.text("MANUFACTURE: " + build.manufacturer);
        BUILD.build_version.text("BUILDVERSION: " + build.buildVersion);
      });
  };
}

function CellularNetworkInfo() {
  this.status = $('<p></p>').text("CELLULAR is OFF");
  this.apn_imei = $('<p></p>');
  this.ip_address = $('<p></p>');
  this.ipv6_address = $('<p></p>');
  this.short_info = $('<p></p>');
  this.bool_info = $('<p></p>');
  $('#cellular_network_info').append(this.status, this.apn_imei,
                                     this.ip_address, this.ipv6_address,
                                     this.short_info, this.bool_info);

  this.change_cellular = function change_cellular(cellular) {
    if (cellular.status == 'ON') {
      this.status.text("CELLULAR: " + cellular.status);
      this.apn_imei.text("APN: " + cellular.apn + "  " +
                         "IMEI: " + cellular.imei);
      this.ip_address.text("IP ADDRESS: " + cellular.ipAddress);
      this.ipv6_address.text("IPV6 ADDRESS: " + cellular.ipv6Address);
      this.short_info.text("MCC: " + cellular.mcc + "  " +
                           "MNC: " + cellular.mnc + "  " +
                           "CELLID: " + cellular.cellId + "  " +
                           "LAC: " + cellular.lac);
      this.bool_info.text(cellular.isRoaming? "Roaming":"NoRoaming" +
                          cellular.isFlightMode? "Flight":"NoFlight");
    } else {
      this.status.text("CELLULAR IS OFF!");
    }
  };

  this.getInfo = function getInfo(CELLULAR) {
    tizen.systeminfo.getPropertyValue(
      "CELLULAR_NETWORK",
      function(cellular) {
        CELLULAR.change_cellular(cellular);
      },
      null);
    tizen.systeminfo.addPropertyValueChangeListener(
      "CELLULAR_NETWORK",
      function(cellular) {
        CELLULAR.change_cellular(cellular);
      });
  };
}


function CPUInfo() {
  this.cpu_container = document.getElementById('cpu_container');
  this.cpu_data = [];
  this.cpu_data_size = 200;
  this.cpu_graph = null;
  this.graph_lines = {
    data: [],
    label: 'CPU',
    lines: {
      show: true,
      fill: true,
      fillColor: {
      colors: ['#f00', '#fdd'],
        getInfo: 'top',
        end: 'bottom',
      },
      fillOpacity: 1,
    }
  };
  this.graph_yaxis = {
    title: 'USAGE',
    noTicks: 6,
    tickFormatter: function(n) {return n + '%';},
    max: 100,
    min: -5,
    color: '#fff',
    autoScale: true,
  };
  this.graph_xaxis = {
    title: 'TIME',
    noTicks: true,
    ticks: [[0, "PAST"], [200, "FUTURE"]],
    max: 200,
    min: 0,
  };
  this.graph_grid = {
    backgroundColor: {
      colors: [[0, '#fff'], [1, '#bbb']],
      getInfo: 'top',
      end: 'bottom',
    }
  };

  this.cpu_animate = function cpu_animate(CPU, length) {
    if (CPU.cpu_data.length <= CPU.cpu_data_size) {
      CPU.cpu_data.push(CPU.cpu_data[length - 1]);
    } else {
      CPU.cpu_data.push(CPU.cpu_data[length - 1]);
      CPU.cpu_data.shift();
    }

    CPU.graph_lines.data = [];
    for (var i = 0; i < CPU.cpu_data.length; ++i) {
      CPU.graph_lines.data.push([i, CPU.cpu_data[i]]);
    }

    while (CPU.cpu_data[length - 1] > CPU.graph_yaxis.max) {
      CPU.graph_yaxis.max += 50;
    }

    CPU.cpu_graph = Flotr.draw(CPU.cpu_container,
      [CPU.graph_lines], {
        title: CPU.cpu_data[length - 1] + '%',
        yaxis: CPU.graph_yaxis,
        xaxis: CPU.graph_xaxis,
        grid: CPU.graph_grid,
        legend: {position: 'nw'},
      }
    );

    setTimeout(function() {
      CPU.cpu_animate(CPU, CPU.cpu_data.length);
      }, 50);
  };

  this.getInfo = function getInfo(CPU) {
    tizen.systeminfo.getPropertyValue(
      "CPU",
      function(cpu) {
        CPU.cpu_data.push((cpu.load * 100).toFixed(1));
      },
      null);
    CPU.cpu_animate(CPU, CPU.cpu_data.length);
    tizen.systeminfo.addPropertyValueChangeListener(
      "CPU",
      function(cpu) {
        if (CPU.cpu_data.length < CPU.cpu_data_size) {
          CPU.cpu_data.push((cpu.load * 100).toFixed(1));
        } else {
          CPU.cpu_data.shift();
          CPU.cpu_data.push((cpu.load * 100).toFixed(1));
        }
      });
  };
}

function DeviceOrientationInfo() {
  this.status = $('<p></p>');
  this.auto_rotation = $('<p></p>');
  $('#deviceorientation').append(this.status, this.auto_rotation);

  this.change_status = function change_status(status) {
    switch(status) {
      case "PORTRAIT_PRIMARY":
        $('#ball').css('-webkit-animation', 'jump-pp 1s infinite');
        $('#ball').css('animation', 'jump-pp 1.5s infinite');
        $('#ball').removeClass();
        $('#ball').addClass('ball-top');
        break;
      case "PORTRAIT_SECONDARY":
        $('#ball').css('-webkit-animation', 'jump-ps 1s infinite');
        $('#ball').css('animation', 'jump-ps 1.5s infinite');
        $('#ball').removeClass();
        $('#ball').addClass('ball-bottom');
        break;
      case "LANDSCAPE_PRIMARY":
        $('#ball').css('-webkit-animation', 'jump-lp 1s infinite');
        $('#ball').css('animation', 'jump-lp 1.5s infinite');
        $('#ball').removeClass();
        $('#ball').addClass('ball-left');
        break;
      case "LANDSCAPE_SECONDARY":
        $('#ball').css('-webkit-animation', 'jump-ls 1s infinite');
        $('#ball').css('animation', 'jump-ls 1.5s infinite');
        $('#ball').removeClass();
        $('#ball').addClass('ball-right');
        break;
      default:
        $('#ball').css('-webkit-animation', 'jump-no 1s infinite');
        $('#ball').css('animation', 'jump-no 1.5s infinite');
        $('#ball').removeClass();
    }
  };

  this.getInfo = function getInfo(DEVICEORIENTATION) {
    tizen.systeminfo.getPropertyValue(
      "DEVICE_ORIENTATION",
      function(deviceorientation) {
        DEVICEORIENTATION.change_status(deviceorientation.status);
        DEVICEORIENTATION.status.text("STATUS: " + deviceorientation.status);
        DEVICEORIENTATION.auto_rotation.text(deviceorientation.isAutoRotation ?
                                             "AUTOROTATION":"NO AUTOROTATION");
      },
      null);
    tizen.systeminfo.addPropertyValueChangeListener(
      "DEVICE_ORIENTATION",
      function(deviceorientation) {
        DEVICEORIENTATION.change_status(deviceorientation.status);
        DEVICEORIENTATION.status.text("STATUS: " + deviceorientation.status);
        DEVICEORIENTATION.auto_rotation.text(deviceorientation.isAutoRotation ?
                                             "AUTOROTATION":"NO AUTOROTATION");
      });
  };
}

function DisplayInfo() {
  this.dots_perinch_width = $('<p></p>').addClass('center');
  this.dots_perinch_height = $('<p></p>').addClass('center');
  this.screen = $('<p></p>').addClass('center');
  this.physical = $('<p></p>').addClass('center');
  this.brightness = $('<p></p>').addClass('center');
  $('#display_status').append(this.dots_perinch_width, this.dots_perinch_height,
                              this.screen, this.physical, this.brightness);

  this.change_display = function change_display(display) {
    this.dots_perinch_width.text("DOS PER INCH WIDTH: " +
        display.dotsPerInchWidth.toFixed(2));
    this.dots_perinch_height.text("DOS PER INCH HEIGTH: " +
        display.dotsPerInchHeight.toFixed(2));
    this.screen.text("SCREEN: WIDTH(" + display.resolutionWidth +
        ") HEIGHT(" + display.resolutionHeight + ")");
    this.physical.text("PHYSICAL: WIDTH(" + display.physicalWidth +
        ") HEIGHT(" + display.physicalHeight + ")");
    this.brightness.text("BRIGHTNESS: " + display.brightness);
    var physicalTotal = display.physicalWidth + display.physicalHeight;
    var screenTotal = display.resolutionWidth + display.resolutionHeight;
    $('#display_physical').width(500 * display.physicalWidth /
        physicalTotal + 'px');
    $('#display_physical').height(500 * display.physicalHeight /
        physicalTotal + 'px');
    $('#display_screen').width(380 * display.resolutionWidth /
        screenTotal + 'px');
    $('#display_screen').height(380 * display.resolutionHeight /
        screenTotal + 'px');
    $('#display_screen').css('opacity', display.brightness);
  };

  this.getInfo = function getInfo(DISPLAY) {
    tizen.systeminfo.getPropertyValue(
      "DISPLAY",
      function(display) {
        DISPLAY.change_display(display);
      },
      null);
    tizen.systeminfo.addPropertyValueChangeListener(
      "DISPLAY",
      function(display) {
        DISPLAY.change_display(display);
      });
  };
}

function LocaleInfo() {
  this.language = $('<p></p>').addClass('center');
  this.country = $('<p></p>').addClass('center');
  $('#locale').append(this.language, this.country);

  this.getInfo = function getInfo(LOCALE) {
    tizen.systeminfo.getPropertyValue(
      "LOCALE",
      function(locale) {
        LOCALE.language.text("LANGUAGE: " + locale.language);
        LOCALE.country.text("COUNTRY: " + locale.country);
      },
      null);
    tizen.systeminfo.addPropertyValueChangeListener(
      "LOCALE",
      function(locale) {
        LOCALE.language.text("LANGUAGE: " + locale.language);
        LOCALE.country.text("COUNTRY: " + locale.country);
      });
  };
}

function NetworkInfo() {
  this.network_type = $('#network_type');

  this.getInfo = function getInfo(NETWORK) {
    tizen.systeminfo.getPropertyValue(
      "NETWORK",
      function(network) {
        NETWORK.network_type.text("TYPE: " + network.type);
      },
      null);
    tizen.systeminfo.addPropertyValueChangeListener(
      "NETWORK",
      function(network) {
        NETWORK.network_type.text("TYPE: " + network.type);
      });
  };
}

function PeripheralInfo() {
  this.video_output = $('<p></p>');
  $('#peripheral').append(this.video_output);

  this.getInfo = function getInfo(PERIPHERAL) {
    tizen.systeminfo.getPropertyValue(
      "PERIPHERAL",
      function(peripheral) {
        PERIPHERAL.video_output.text(peripheral.isVideoOutputOn ?
                                     "VIDEOOUTPUT: ON":"VIDEOOUTPUT: OFF");
      },
      null);
    tizen.systeminfo.addPropertyValueChangeListener(
      "PERIPHERAL",
      function(peripheral) {
        PERIPHERAL.video_output.text(peripheral.isVideoOutputOn ?
                                     "VIDEOOUTPUT: ON":"VIDEOOUTPUT: OFF");
      });
  };
}

function SimInfo() {
  this.state = $('<p></p>');
  this.operator_name = $('<p></p>');
  this.msisdn_iccid = $('<p></p>');
  this.short_info = $('<p></p>');
  this.msin_spn = $('<p></p>');
  $('#sim_container').append(this.state, this.operator_name, this.msisdn_iccid,
                             this.short_info, this.msin_spn);

  this.change_sim = function change_sim(sim) {
    this.state.text("STATE: " + sim.state);
    this.operator_name.text("OPERATOR NAME: " + sim.operatorName);
    this.msisdn_iccid.text("MSISDN: " + sim.msisdn + "  " +
        "ICCID: " + sim.iccid);
    this.short_info.text("MCC: " + sim.mcc + "  " +
        "MNC: " + sim.mnc);
    this.msin_spn.text("MSIN: " + sim.msin + "  " +
        "SPN: " + sim.spn);
  };

  this.getInfo = function getInfo(SIM) {
    tizen.systeminfo.getPropertyValue(
      "SIM",
      function(sim) {
        SIM.change_sim(sim);
      },
      null);
    tizen.systeminfo.addPropertyValueChangeListener(
      "SIM",
      function(sim) {
        SIM.change_sim(sim);
      });
  };
}

function StorageInfo() {
  this.draw_graph = function draw_graph(UNITS) {
    $('#storage_container').empty();
    for (var i = 0; i < UNITS.length; ++i) {
      var unit = $('<div></div>').attr('id', 'unit' + i);
      unit.width('75%');
      unit.height(85 / UNITS.length + '%');
      unit.css('margin', '0 auto');
      unit.css('padding', '10px');
      $('#storage_container').css('position', 'relative');
      $('#storage_container').append(unit);
      var data1 = [0, UNITS[i].capacity - UNITS[i].availableCapacity];
      var data2 = [0, UNITS[i].availableCapacity];
      var containner = document.getElementById('unit' + i);
      Flotr.draw(containner, [
        {data: [data1], label: 'used'},
        {data: [data2], label: 'available'}
        ], {
          HtmlText: false,
          title: 'TYPE:' + UNITS[i].type,
          grid: {
            verticalLines: false,
            horizontalLines: false,
            circular: true,
          },
          xaxis: {showLabels: false},
          yaxis: {showLabels: false},
          pie: {
            show: true,
            explode: 6,
          },
          mouse: {track: true},
          legend: {
            position: 'se',
            backgroundColor: '#D2E8FF',
          }
        });
    }
  };

  this.getInfo = function getInfo(STORAGE) {
    tizen.systeminfo.getPropertyValue(
      "STORAGE",
      function(storage) {
        STORAGE.draw_graph(storage.units);
      },
      null);
    tizen.systeminfo.addPropertyValueChangeListener(
      "STORAGE",
      function(storage) {
        STORAGE.draw_graph(storage.units);
      });
  };
}

function WifiNetworkInfo() {
  this.status = $('<p></p>').text("WIFI IS OFF!");
  this.ssid = $('<p></p>');
  this.ip_address = $('<p></p>');
  this.ipv6_address = $('<p></p>');
  this.signal_strength = $('<p></p>');
  $('#wifi_network_info').append(this.status, this.ssid, this.ip_address,
                                 this.ipv6_address, this.signal_strength);

  this.change_wifi = function change_wifi(wifi) {
    if (wifi.status == 'ON') {
      this.status.text("WIFI: " + wifi.status);
      this.ssid.text("WIFI SSID: " + wifi.ssid);
      this.ip_address.text("WIFI IP ADDRESS: " + wifi.ipAddress);
      this.ipv6_address.text("WIFI IPV6 ADDRESS: " + wifi.ipv6Address);
      this.signal_strength.text("WIFI SIGNAL STRENGTH: " +
                                wifi.signalStrength);
    } else {
      this.status.text("WIFI IS OFF!");
    }
  };

  this.getInfo = function getInfo(WIFI) {
    tizen.systeminfo.getPropertyValue(
      "WIFI_NETWORK",
      function(wifi) {
        WIFI.change_wifi(wifi);
      },
      null);
    tizen.systeminfo.addPropertyValueChangeListener(
      "WIFI_NETWORK",
      function(wifi) {
        WIFI.change_wifi(wifi);
      });
  };
}

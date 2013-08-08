Number.prototype.format = function() {
    if (this < 10)
        return "0" + this;
    return this;
}

var blueApp = {};

/**
 * Initializes the application data and settings.
 */
blueApp.init = function() {
    $("#return-btn").click(function(event) {
	// TODO use tizen.application whenever it's fully implemented
        //var currentApp = tizen.application.getCurrentApplication();
        //currentApp.exit();
	window.close();
    });

    blueApp.cleanDeviceList();

    $("#visibility-toggle").click(blueApp.toggleVisibility);

    $("input[type=radio][name=visibility]").change(function() {
        blueApp.changedVisibility();
    });

    $("#scan-btn").click(blueApp.scan);

    blueApp.bluetoothLoad();

    $("#blueetooth-toggle").change(blueApp.adapterStatusToggle);
};

/**
 * On power setting success callback.
 */
blueApp.adapterPowerSuccessCb = function() {
    var status = $("#blueetooth-toggle").val();

    var noti = new tizen.StatusNotification("SIMPLE", "Bluetooth", {
        content : "Successfully powered " + status + " bluetooth adapter."
    });
    tizen.notification.post(noti);
    blueApp.discoverDevices();
};

/**
 * On power setting error callback.
 */
blueApp.adapterPowerErrCb = function() {
    var status = $("#blueetooth-toggle").val();

    var noti = new tizen.StatusNotification("SIMPLE", "Bluetooth", {
        content : "Failed to power " + status + " bluetooth adapter."
    });
    tizen.notification.post(noti);
};

/**
 * Device buttom has been toggled.
 *
 * Powers down the bluetooth device.
 */
blueApp.adapterStatusToggle = function() {
    var status = $("#blueetooth-toggle").val() == 'on';

    if (!status) {
        blueApp.cleanDeviceList();
        blueApp.adapter.stopDiscovery(function() {
            blueApp.adapter.setPowered(status, blueApp.adapterPowerSuccessCb,
                    blueApp.adapterPowerErrCb);
        });
	blueApp.adapter = tizen.bluetooth.getDefaultAdapter();
    } else {
        blueApp.adapter.setPowered(status, blueApp.adapterPowerSuccessCb,
                blueApp.adapterPowerErrCb);
	blueApp.adapter = null;
    }
};

/**
 * Clean the device list.
 */
blueApp.cleanDeviceList = function() {
    $("#device-group").hide();
    $("#device-group").html('');
};

/**
 * Scan for bluetooth devices.
 */
blueApp.scan = function() {
    if (!blueApp.adapter) return;

    blueApp.adapter.stopDiscovery(function() {
	blueApp.cleanDeviceList();
	blueApp.discoverDevices();
    });
};

/**
 * Efectively add new devices to devices list.
 */
blueApp.addDevice = function(device) {
    var deviceStyle;
    var deviceClass = device.deviceClass;
    var deviceItem = "<li><div id='device-icon' ";

    if (!blueApp.adapter.powered) return;

    switch (deviceClass.major) {
    case tizen.bluetooth.deviceMajor.COMPUTER:
        deviceStyle = "device-icon-computer";
        break;
    case tizen.bluetooth.deviceMajor.PHONE:
        deviceStyle = "device-icon-telephone";
        break;
    case tizen.bluetooth.deviceMajor.IMAGING:
        if (deviceClass.minor == tizen.bluetooth.deviceMinor.PRINTER) {
            deviceStyle = "device-icon-printer";
        }
        break;
    }

    deviceItem += "class='" + deviceStyle + "' />";
    deviceItem += device.name;
    deviceItem += "</li>";

    $("#device-group").show();
    $("#device-group").append(
            $(deviceItem).attr("class",
                    "ui-li ui-li-static ui-btn-up-s ui-li-last").attr(
                    "address", device.address));
};

/**
 * Tizen bluetooth callbacs.
 */
blueApp.discoverDevicesCb = {
    onstarted: function() {
            console.log("Discovery has started.");
    },
    ondevicefound : blueApp.addDevice,
    ondevicedisappeared : function(address) {
        $("#device-group li").each(function() {
            if ($(this).attr("address") == address)
                $(this).remove();
        });
    },
    onfinished : function(devices) {
        var noti = new tizen.StatusNotification("SIMPLE", "Bluetooth", {
            content : "Finished bluetooth devices discovery."
        });
        tizen.notification.post(noti);
    }
};

/**
 * Start the discover call and handle failure.
 */
blueApp.discoverDevices = function() {
    blueApp.adapter.discoverDevices(blueApp.discoverDevicesCb, function(e) {
        var noti = new tizen.StatusNotification("SIMPLE", "Bluetooth", {
            content : "Failed to discover bluetooth devices."
        });
        tizen.notification.post(noti);
    });
};

/**
 * Bluetooth initial data load.
 *
 * Gets the current default adapter and start to discover devices if it's
 * powered.
 */
blueApp.bluetoothLoad = function() {
    try {
	blueApp.adapter = tizen.bluetooth.getDefaultAdapter();
	$("#adapter-name").html(blueApp.adapter.name);
    } catch(err) {
	console.log("bluetooth is off");
    }

    if (!blueApp.adapter || !blueApp.adapter.powered) return;

    if (blueApp.adapter.visible) {
        $("#visibility-display").html("On");
    }

    $("#blueetooth-toggle").val("on");
    $("#blueetooth-toggle").slider('refresh');

    blueApp.discoverDevices();
};

/**
 * Timer for setting the UI visible remaining time case a timeout has been set.
 */
blueApp.setRemainingTime = function() {
    var timeOut;
    var delta;
    var display;
    var currTime = new Date().getTime() / 1000;
    var visibilityTime = blueApp.visibilityTime;
    var visibilityTimeout = blueApp.visibilityTimeout;

    timeOut = visibilityTime + visibilityTimeout;
    delta = timeOut - currTime;
    display = Math.floor(delta / 60).format() + ":"
            + Math.floor(delta % 60).format();

    $("#visibility-display").html(display);

    if (currTime < timeOut)
        setTimeout(blueApp.setRemainingTime, 1000);
    else
        $("#visibility-display").html("Off");
};

/**
 * On visibility setting success callback.
 */
blueApp.visibilitySuccessCb = function() {
    setTimeout(blueApp.setRemainingTime, 1000);
};

/**
 * On visibility setting error callback.
 */
blueApp.visibilityErrorCb = function() {
    var noti = new tizen.StatusNotification("SIMPLE", "Bluetooth", {
        content : "Error on setting bluetooth visibility."
    });
    tizen.notification.post(noti);
    $("#visibility-display").html("Off");
};

/**
 * Reset the visibility label.
 *
 * Sets the visibility label with the just selected visibility item.
 */
blueApp.changedVisibility = function() {
    var timeout = parseInt($("input[type=radio][name=visibility]:checked")
            .val(), 10);
    var visibility = $("input[type=radio][name=visibility]:checked").attr(
            "label");
    blueApp.toggleVisibility();
    $("#visibility-display").html(visibility);
    blueApp.visibilityTime = 0;
    blueApp.visibilityTimeout = 0;

    if (timeout > 0) {
        blueApp.visibilityTime = new Date().getTime() / 1000;
        blueApp.visibilityTimeout = timeout;

        blueApp.adapter.setVisible(true, blueApp.visibilitySuccessCb,
                blueApp.visibilityErrorCb, timeout);
    } else if (timeout == 0)
        blueApp.adapter.setVisible(false);
    else
        blueApp.adapter.setVisible(true);
};

/**
 * Toggle the device/adapter visibility.
 *
 * Displays or not the visibility options(Off, 1 min, 5 min etc) and toggles the
 * the item arrow(off/on).
 */
blueApp.toggleVisibility = function() {
    var vGroup = $("#visibility-group");
    var display = "none";
    var toggle = "visibility-on";

    if (!blueApp.adapter) return;

    if (vGroup.css("display") == "none") {
        display = "block";
        toggle = "visibility-off";
    }

    vGroup.css("display", display);
    $("#visibility-toggle-icon").attr("class", toggle);
};

$(document).ready(blueApp.init);

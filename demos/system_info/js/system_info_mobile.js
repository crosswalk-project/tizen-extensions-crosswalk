$(document).ready(function () {
  $('#exit_btn').click(function() {
    // Open the local windows. To avoid window.close() invalid problem.
    window.open('', '_self', '');
    window.close();
  });

  $('#main').ascensor({
    direction: "chocolate",
    overflow: 'hidden',
    loop: true,
    ascensorMap: [[0,0],[0,1],[1,1],[1,0],[2,0],[2,1],[3,1],[3,0]]
  });

  battery_info = new BatteryInfo();
  battery_info.getInfo(battery_info);

  build_info = new BuildInfo();
  build_info.getInfo(build_info);

  cellular_network_info = new CellularNetworkInfo();
  cellular_network_info.getInfo(cellular_network_info);

  cpu_info = new CPUInfo();
  cpu_info.getInfo(cpu_info);

  deviceorientation_info = new DeviceOrientationInfo();
  deviceorientation_info.getInfo(deviceorientation_info);

  display_info = new DisplayInfo();
  display_info.getInfo(display_info);

  locale_info = new LocaleInfo();
  locale_info.getInfo(locale_info);

  network_info = new NetworkInfo();
  network_info.getInfo(network_info);

  peripheral_info = new PeripheralInfo();
  peripheral_info.getInfo(peripheral_info);

  sim_info = new SimInfo();
  sim_info.getInfo(sim_info);

  storage_info = new StorageInfo();
  storage_info.getInfo(storage_info);

  wifi_network_info = new WifiNetworkInfo();
  wifi_network_info.getInfo(wifi_network_info);

});

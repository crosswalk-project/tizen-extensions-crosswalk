$(document).ready(function () {
  $('#main').ascensor({
    direction: "chocolate",
    overflow: 'hidden',
    loop: true,
    ascensorMap: [[0,0],[0,1],[0,2],[1,2],[1,1],[1,0]]
  });

  build_info = new BuildInfo();
  build_info.getInfo(build_info);

  cpu_info = new CPUInfo();
  cpu_info.getInfo(cpu_info);

  display_info = new DisplayInfo();
  display_info.getInfo(display_info);

  locale_info = new LocaleInfo();
  locale_info.getInfo(locale_info);

  network_info = new NetworkInfo();
  network_info.getInfo(network_info);

  storage_info = new StorageInfo();
  storage_info.getInfo(storage_info);

  wifi_network_info = new WifiNetworkInfo();
  wifi_network_info.getInfo(wifi_network_info);
});

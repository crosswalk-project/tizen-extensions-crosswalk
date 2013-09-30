{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_system_info',
      'type': 'loadable_module',
      'conditions': [
        [ 'extension_host_os == "desktop"', {
          'variables': {
            'packages': [
              'gio-2.0',
              'NetworkManager',
            ]
          },
        }],
        [ 'extension_host_os == "mobile"', {
          'variables': {
            'packages': [
              'appcore-common',
              'capi-network-connection',
              'capi-system-info',
              'capi-system-runtime-info',
              'capi-system-sensor',
              'capi-telephony-sim',
              'pkgmgr-info',
              'vconf',
            ]
          },
        }],
      ],
      'variables': {
        'packages': [
          'dbus-glib-1',
          'glib-2.0',
          'libudev',
        ]
      },
      'ldflags': [
        '-lX11',
      ],
      'includes': [
        '../common/pkg-config.gypi',
      ],
      'sources': [
        'system_info_api.js',
        'system_info_battery.h',
        'system_info_battery_desktop.cc',
        'system_info_battery_mobile.cc',
        'system_info_build.h',
        'system_info_build_desktop.cc',
        'system_info_build_mobile.cc',
        'system_info_cellular_network.h',
        'system_info_cellular_network_desktop.cc',
        'system_info_cellular_network_mobile.cc',
        'system_info_context.cc',
        'system_info_context.h',
        'system_info_cpu.cc',
        'system_info_cpu.h',
        'system_info_device_orientation.h',
        'system_info_device_orientation_desktop.cc',
        'system_info_device_orientation_mobile.cc',
        'system_info_display.h',
        'system_info_display_x11.cc',
        'system_info_locale.h',
        'system_info_locale_desktop.cc',
        'system_info_locale_mobile.cc',
        'system_info_network.cc',
        'system_info_network.h',
        'system_info_network_desktop.cc',
        'system_info_network_mobile.cc',
        'system_info_peripheral.h',
        'system_info_peripheral_desktop.cc',
        'system_info_peripheral_mobile.cc',
        'system_info_sim.h',
        'system_info_sim_desktop.cc',
        'system_info_sim_mobile.cc',
        'system_info_storage.cc',
        'system_info_storage.h',
        'system_info_storage_desktop.cc',
        'system_info_storage_mobile.cc',
        'system_info_utils.cc',
        'system_info_utils.h',
        'system_info_wifi_network.cc',
        'system_info_wifi_network.h',
        'system_info_wifi_network_desktop.cc',
        'system_info_wifi_network_mobile.cc',
      ],
    },
  ],
}

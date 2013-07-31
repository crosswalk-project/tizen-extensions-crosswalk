{
  'targets': [
    {
      'target_name': 'tizen_system_info',
      'type': 'loadable_module',
      'variables': {
        'packages': [
          'glib-2.0',
          'libudev',
        ]
      },
      'includes': [
        '../pkg-config.gypi',
      ],
      'sources': [
        'system_info_api.js',
        'system_info_battery.cc',
        'system_info_battery.h',
        'system_info_build.cc',
        'system_info_build.h',
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
        'system_info_locale.cc',
        'system_info_locale.h',
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
        'system_info_utils.cc',
        'system_info_utils.h',
        'system_info_wifi_network.h',
        'system_info_wifi_network_desktop.cc',
        'system_info_wifi_network_mobile.cc',
      ],
    },
  ],
}
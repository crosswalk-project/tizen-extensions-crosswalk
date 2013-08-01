{
  'targets': [
    {
      'target_name': 'tizen_system_info',
      'type': 'loadable_module',
      'variables': {
        'packages': [
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
        'system_info_context.cc',
        'system_info_context.h',
        'system_info_context_desktop.cc',
        'system_info_context_mobile.cc',
        'system_info_display.h',
        'system_info_display_x11.cc',
        'system_info_storage.cc',
        'system_info_storage.h',
        'system_info_utils.cc',
        'system_info_utils.h',
      ],
    },
  ],
}

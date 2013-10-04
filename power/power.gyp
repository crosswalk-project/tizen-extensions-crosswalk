{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_power',
      'type': 'loadable_module',
      'sources': [
        'mobile/power_event_source.cc',
        'mobile/power_event_source.h',
        'power_api.js',
        'power_extension.cc',
        'power_extension.h',
        'power_instance_desktop.cc',
        'power_instance_desktop.h',
        'power_instance_mobile.cc',
        'power_instance_mobile.h',
        'power_types.h',
        '../common/extension.h',
        '../common/extension.cc',
      ],
      'includes': [
        '../common/pkg-config.gypi',
      ],
      'conditions': [
        ['extension_host_os=="mobile"', {
          'variables': {
            'packages': [
              'glib-2.0',
              'capi-system-device',
              'capi-system-power',
              'pmapi',
              'vconf',
            ]
          },
        }],
        ['extension_host_os == "desktop"', {
          'variables': {
            'packages': [
              'gio-2.0',
            ]
          },
        }],
      ],
    },
  ],
}

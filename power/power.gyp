{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_power',
      'type': 'loadable_module',
      'sources': [
        'power_api.js',
        'power_context.cc',
        'power_context.h',
        'power_context_desktop.cc',
        'power_context_mobile.cc',
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

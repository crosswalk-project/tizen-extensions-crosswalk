{
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
      'conditions': [
        ['type=="mobile"', {
          'dependencies': [
            'capi-system-device',
            'capi-system-power',
            'pmapi',
            'vconf',
          ],
        }],
        [ 'type == "desktop"', {
            'variables': { 'packages': ['gio-2.0'] },
            'includes': [ '../pkg-config.gypi' ],
        }],
      ],
    },
  ],
}

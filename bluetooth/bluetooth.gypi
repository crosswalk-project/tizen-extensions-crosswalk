{
  'targets': [
    {
      'target_name': 'tizen_bluetooth',
      'type': 'loadable_module',
      'sources': [
        'bluetooth_api.js',
        'bluetooth_context.cc',
        'bluetooth_context.h',
        'bluetooth_context_desktop.cc',
      ],

      'conditions': [
        [ 'type == "desktop"', {
            'variables': { 'packages': ['gio-2.0'] },
            'includes': [ '../pkg-config.gypi' ],
          }
        ],
      ],
    },
  ],
}

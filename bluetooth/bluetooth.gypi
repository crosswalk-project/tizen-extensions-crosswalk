{
  'targets': [
    {
      'variables': {
        'packages': ['gio-2.0'],
        'bluetooth%': 'bluez4',
      },

      'includes': [ '../pkg-config.gypi' ],
      'target_name': 'tizen_bluetooth',
      'type': 'loadable_module',
      'sources': [
        'bluetooth_api.js',
        'bluetooth_context.cc',
        'bluetooth_context.h',
      ],

      'conditions': [
        [ 'bluetooth == "bluez5"', {
            'sources': ['bluetooth_context_bluez5.cc'],
            'defines': ['BLUEZ_5'],
          }
        ],
        [ 'bluetooth == "bluez4"', {
            'sources': ['bluetooth_context_bluez4.cc'],
            'defines': ['BLUEZ_4'],
          }
        ],
      ],
    },
  ],
}

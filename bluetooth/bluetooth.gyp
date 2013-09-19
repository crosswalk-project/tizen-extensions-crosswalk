{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_bluetooth',
      'type': 'loadable_module',
      'variables': {
        'packages': [
          'gio-2.0',
          'bluez',
        ],
        'bluetooth%': 'bluez4',
      },
      'includes': [
        '../common/pkg-config.gypi',
      ],
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
        [ 'extension_host_os == "mobile"', {
            'variables': { 'packages': ['capi-network-bluetooth'] },
        }],
      ],
    },
  ],
}

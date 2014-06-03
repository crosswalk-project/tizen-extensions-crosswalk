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
        'bluetooth_extension.cc',
        'bluetooth_extension.h',
        'bluetooth_instance.cc',
        'bluetooth_instance.h',
      ],
      'conditions': [
        [ 'bluetooth == "bluez5"', {
            'sources': ['bluetooth_instance_bluez5.cc'],
            'defines': ['BLUEZ_5'],
          }
        ],
        [ 'bluetooth == "bluez4"', {
            'sources': ['bluetooth_instance_bluez4.cc'],
            'defines': ['BLUEZ_4'],
          }
        ],
        [ 'tizen == 1', {
            'variables': { 'packages': ['capi-network-bluetooth'] },
        }],
      ],
    },
  ],
}

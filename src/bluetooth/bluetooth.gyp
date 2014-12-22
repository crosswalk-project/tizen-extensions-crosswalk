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
        'bluetooth%': '<!(bash identify_bluetooth_type.sh)',
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
        [ 'bluetooth == "bluez5" or bluetooth == "bluez4"', {
          'variables': {
            'packages': [
             'gio-2.0',
             'bluez',
           ],
          },
        }],
        [ 'bluetooth == "bluez5"', {
            'sources': [
              'bluetooth_instance_bluez5.cc',
            ],
            'defines': ['BLUEZ_5'],
          }
        ],
        [ 'bluetooth == "bluez4"', {
            'sources': [
              'bluetooth_instance_bluez4.cc',
            ],
            'defines': ['BLUEZ_4'],
          }
        ],
        [ 'bluetooth == "tizen_capi"', {
            'sources!': [
              'bluetooth_instance.cc',
              'bluetooth_instance.h',
            ],
            'sources': [
              'bluetooth_instance_capi.cc',
              'bluetooth_instance_capi.h',
            ],
            'variables': {
              'packages': [
                'capi-network-bluetooth',
                'glib-2.0',
              ]
            },
            'defines': ['TIZEN_CAPI_BT'],
          }
        ],
      ],
    },
  ],
}

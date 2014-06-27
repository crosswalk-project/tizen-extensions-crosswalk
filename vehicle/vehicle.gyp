{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_vehicle',
      'type': 'loadable_module',
      'variables': {
        'packages': [
          'automotive-message-broker',
          'icu-i18n',
          'gio-2.0',
          'gio-unix-2.0',
        ],
      },
      'includes': [
        '../common/pkg-config.gypi',
      ],
      'sources': [
        'vehicle.cc',
        'vehicle.h',
        'vehicle_api.js',
        'vehicle_extension.cc',
        'vehicle_extension.h',
        'vehicle_instance.cc',
        'vehicle_instance.h',
      ],
      'conditions': [
        [ 'tizen == 1', {
            'variables': { 'packages': ['vconf'] },
        }],
      ],
    },
  ],
}

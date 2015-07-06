{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_iotivity',
      'type': 'loadable_module',
      'variables': {
        'packages': [
        ],
      },
      'sources': [
        'iotivity_api.js',
        'iotivity_extension.cc',
        'iotivity_extension.h',
        'iotivity_instance.cc',
        'iotivity_instance.h',
      ],
      'includes': [
        '../common/pkg-config.gypi',
      ],
    },
  ],
}

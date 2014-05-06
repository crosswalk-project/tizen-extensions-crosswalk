{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_phone',
      'type': 'loadable_module',
      'variables': {
        'packages': [
          'gio-2.0',
        ],
      },
      'includes': [
        '../common/pkg-config.gypi',
      ],
      'sources': [
        'phone_api.js',
        'phone_extension.cc',
        'phone_extension.h',
        'phone_instance.cc',
        'phone_instance.h',
      ],
    },
  ],
}

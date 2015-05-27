{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_telephony',
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
        'telephony_api.js',
        'telephony_backend_ofono.cc',
        'telephony_backend_ofono.h',
        'telephony_extension.cc',
        'telephony_extension.h',
        'telephony_instance.cc',
        'telephony_instance.h',
        'telephony_logging.h',
      ],
    },
  ],
}

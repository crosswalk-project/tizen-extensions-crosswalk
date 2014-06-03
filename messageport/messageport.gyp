{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_messageport',
      'type': 'loadable_module',
      'variables': {
        'packages': [
          'bundle',
          'message-port',
        ],
      },
      'includes': [
        '../common/pkg-config.gypi',
      ],
      'sources': [
        'messageport_api.js',
        'messageport_extension.cc',
        'messageport_extension.h',
        'messageport_instance.cc',
        'messageport_instance.h',
      ],
    },
  ],
}

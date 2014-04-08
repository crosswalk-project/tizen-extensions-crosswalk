{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_speech',
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
        'speech_api.js',
        'speech_extension.cc',
        'speech_extension.h',
        'speech_instance.cc',
        'speech_instance.h',
        '../common/extension.cc',
        '../common/extension.h',        
      ],
    },
  ],
}

{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_content',
      'type': 'loadable_module',
      'variables': {
        'packages': [
          'capi-content-media-content'
        ],
      },
      'sources': [
        'content_api.js',
        'content_extension.h',
        'content_extension.cc',
        'content_instance.h',
        'content_instance.cc',
        '../common/extension.h',
        '../common/extension.cc',
      ],
      'includes': [
        '../common/pkg-config.gypi',
      ],
    }
  ]
}

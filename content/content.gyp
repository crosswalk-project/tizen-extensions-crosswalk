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
        'content_extension.cc',
        'content_extension.h',
        'content_filter.cc',
        'content_filter.h',
        'content_instance.cc',
        'content_instance.h',
      ],
      'includes': [
        '../common/pkg-config.gypi',
      ],
    }
  ]
}

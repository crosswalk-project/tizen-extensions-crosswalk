{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_filesystem',
      'type': 'loadable_module',
      'variables': {
        'packages': [
          'capi-appfw-application',
        ],
      },
      'sources': [
        'filesystem_api.js',
        'filesystem_context.cc',
        'filesystem_context.h',
      ],
      'includes': [
        '../common/pkg-config.gypi',
      ],
    },
  ],
}

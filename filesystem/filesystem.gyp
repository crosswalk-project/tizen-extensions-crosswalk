{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_filesystem',
      'type': 'loadable_module',
      'sources': [
        'filesystem_api.js',
        'filesystem_context.cc',
        'filesystem_context.h',
      ],
    },
  ],
}

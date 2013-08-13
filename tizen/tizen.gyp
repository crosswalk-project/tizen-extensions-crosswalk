{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen',
      'type': 'loadable_module',
      'sources': [
        'tizen.h',
        'tizen_api.js',
        'tizen_context.cc',
        'tizen_context.h',
      ],
    },
  ],
}

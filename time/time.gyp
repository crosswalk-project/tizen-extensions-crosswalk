{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_time',
      'type': 'loadable_module',
      'sources': [
        'time_api.js',
        'time_context.cc',
      ],
    },
  ],
}

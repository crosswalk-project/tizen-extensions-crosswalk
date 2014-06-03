{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_utils',
      'type': 'loadable_module',
      'sources': [
        'utils_api.js',
        'utils_extension.cc',
        'utils_extension.h',
      ],
    },
  ],
}

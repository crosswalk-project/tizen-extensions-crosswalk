{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_time',
      'type': 'loadable_module',
      'variables': {
        'packages': [
          'icu-i18n',
        ],
      },
      'includes': [
        '../common/pkg-config.gypi',
      ],
      'sources': [
        'time_api.js',
        'time_context.cc',
      ],
      'conditions': [
        [ 'extension_host_os == "mobile"', {
            'variables': { 'packages': ['vconf'] },
        }],
      ],
    },
  ],
}

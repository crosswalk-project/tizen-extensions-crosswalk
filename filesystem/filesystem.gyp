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
          'pkgmgr-info',
        ],
      },
      'sources': [
        'filesystem_api.js',
        'filesystem_extension.cc',
        'filesystem_extension.h',
        'filesystem_instance.cc',
        'filesystem_instance.h',
      ],
      'includes': [
        '../common/pkg-config.gypi',
      ],
    },
  ],
}

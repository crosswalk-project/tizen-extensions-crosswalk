{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_datasync',
      'type': 'loadable_module',
      'variables': {
        'packages': [
          'sync-agent',
        ],
      },
      'dependencies': [
        '../tizen/tizen.gyp:tizen',
      ],
      'includes': [
        '../common/pkg-config.gypi',
      ],
      'sources': [
        'datasync_api.js',
        'datasync_error.h',
        'datasync_extension.cc',
        'datasync_extension.h',
        'datasync_instance.cc',
        'datasync_instance.h',
        'datasync_log.h',
        'datasync_manager.cc',
        'datasync_manager.h',
        'datasync_scoped_exit.h',
        'datasync_serialization.h',
        'sync_info.cc',
        'sync_info.h',
        'sync_profile_info.cc',
        'sync_profile_info.h',
        'sync_service_info.cc',
        'sync_service_info.h',
        'sync_statistics.cc',
        'sync_statistics.h',
      ],
    },
  ],
}

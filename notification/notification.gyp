{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_notification',
      'type': 'loadable_module',
      'includes': [
        '../common/pkg-config.gypi',
      ],
      'sources': [
        'notification_api.js',
        'notification_extension.cc',
        'notification_extension.h',
        'notification_instance_desktop.cc',
        'notification_instance_desktop.h',
        'notification_instance_tizen.cc',
        'notification_instance_tizen.cc',
        'notification_manager_tizen.cc',
        'notification_manager_tizen.h',
        'notification_parameters.cc',
        'notification_parameters.h',
        'picojson_helpers.cc',
        'picojson_helpers.h',
      ],

      'conditions': [
        ['extension_host_os == "desktop"', {
          'variables': { 'packages': ['libnotify'] },
        }],
        ['tizen == 1', {
          'variables': { 'packages': ['notification'] },
        }],
      ],
    },
  ],
}

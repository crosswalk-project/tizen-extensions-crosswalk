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
        'notification_instance_mobile.cc',
        'notification_instance_mobile.h',
        'notification_parameters.cc',
        'notification_parameters.h',
        'mobile/notification_manager.cc',
        'mobile/notification_manager.h',
        'picojson_helpers.h',
        '../common/extension.h',
        '../common/extension.cc',
      ],

      'conditions': [
        [ 'extension_host_os == "desktop"', {
            'variables': { 'packages': ['libnotify'] },
        }],
        [ 'extension_host_os == "mobile"', {
            'variables': { 'packages': ['notification'] },
        }],
      ],
    },
  ],
}

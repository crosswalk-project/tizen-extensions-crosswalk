{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_systemsetting',
      'type': 'loadable_module',
      'sources': [
        'system_setting_api.js',
        'system_setting_extension.cc',
        'system_setting_extension.h',
        'system_setting_instance.cc',
        'system_setting_instance.h',
        'system_setting_instance_desktop.cc',
        'system_setting_instance_tizen.cc',
        'system_setting_locale.cc',
        'system_setting_locale.h',
      ],
      'conditions': [
        ['tizen == 1', {
          'includes': [
            '../common/pkg-config.gypi',
          ],
          'variables': {
            'packages': [
              'capi-system-system-settings',
              'vconf',
            ]
          },
          'includes': [
            '../common/pkg-config.gypi',
          ],
        }],
      ],
    },
  ],
}

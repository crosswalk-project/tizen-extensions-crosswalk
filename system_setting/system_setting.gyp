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
        'system_setting_context.cc',
        'system_setting_context.h',
        'system_setting_context_desktop.cc',
        'system_setting_context_mobile.cc',
      ],
      'conditions': [
        ['extension_host_os=="mobile"', {
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

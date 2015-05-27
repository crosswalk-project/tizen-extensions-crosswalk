{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_websetting',
      'type': 'loadable_module',
      'sources': [
        'web_setting.cc',
        'web_setting.h',
        'web_setting_api.js',
        'web_setting_extension.cc',
        'web_setting_extension.h',
        'web_setting_extension_utils.h',
        'web_setting_instance.cc',
        'web_setting_instance.h',
      ],
      'includes': [
        '../common/pkg-config.gypi',
      ],
      'variables': {
        'packages': [
          'glib-2.0',
        ]
      },
    },
  ],
}

{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_alarm',
      'type': 'loadable_module',
      'sources': [
        'alarm_api.js',
        'alarm_extension.cc',
        'alarm_extension.h',
        'alarm_info.cc',
        'alarm_info.h',
        'alarm_instance.cc',
        'alarm_instance.h',
        'alarm_manager.cc',
        'alarm_manager.h',
      ],
      'conditions': [
        ['tizen == 1', {
          'includes': [
            '../common/pkg-config.gypi',
          ],
          'variables': {
            'packages': [
              'appcore-common',
              'capi-appfw-application',
            ]
          },
        }],
      ],
    },
  ],

}

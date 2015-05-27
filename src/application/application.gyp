{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_application',
      'type': 'loadable_module',
      'sources': [
        'application.cc',
        'application.h',
        'application_api.js',
        'application_context.cc',
        'application_context.h',
        'application_control.cc',
        'application_control.h',
        'application_control_data.cc',
        'application_control_data.h',
        'application_extension.cc',
        'application_extension.h',
        'application_extension_utils.h',
        'application_information.cc',
        'application_information.h',
        'application_instance.cc',
        'application_instance.h',
        'application_manager.cc',
        'application_manager.h',
        'application_requested_control.cc',
        'application_requested_control.h',
      ],
      'conditions': [
        ['tizen == 1', {
          'includes': [
            '../common/pkg-config.gypi',
          ],
          'variables': {
            'packages': [
              'appcore-common',
              'capi-appfw-app-manager',
              'capi-appfw-application',
              'capi-appfw-package-manager',
              'pkgmgr',
              'pkgmgr-info',
            ]
          },
        }],
      ],
    },
  ],
}

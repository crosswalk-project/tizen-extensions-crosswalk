{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_application',
      'type': 'loadable_module',
      'sources': [
        '../common/extension.cc',
        '../common/extension.h',
        'application_api.js',
        'application_context.cc',
        'application_context.h',
        'application_dbus_agent.cc',
        'application_dbus_agent.h',
        'application_extension.cc',
        'application_extension.h',
        'application_information.cc',
        'application_information.h',
        'application_instance.cc',
        'application_instance.h',
        'application_manager.cc',
        'application_manager.h',
      ],
      'conditions': [
        ['tizen == 1', {
          'includes': [
            '../common/pkg-config.gypi',
          ],
          'variables': {
            'packages': [
              'capi-appfw-package-manager',
              'capi-appfw-app-manager',
              'pkgmgr',
              'pkgmgr-info',
            ]
          },
        }],
      ],
    },
  ],
}
